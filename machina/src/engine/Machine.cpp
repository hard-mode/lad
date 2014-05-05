/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <map>

#include "sord/sordmm.hpp"

#include "machina/Atom.hpp"
#include "machina/Context.hpp"
#include "machina/Machine.hpp"
#include "machina/URIs.hpp"
#include "machina/Updates.hpp"
#include "machina/types.hpp"

#include "Edge.hpp"
#include "Node.hpp"
#include "LearnRequest.hpp"
#include "MidiAction.hpp"

using namespace std;
using namespace Raul;

namespace machina {

Machine::Machine(TimeUnit unit)
	: fitness(0.0)
	, _initial_node(new Node(TimeStamp(unit, 0, 0), true))
	, _active_nodes(MAX_ACTIVE_NODES, SPtr<Node>())
	, _time(unit, 0, 0)
	, _is_finished(false)
{
	_nodes.insert(_initial_node);
}

void
Machine::assign(const Machine& copy)
{
	std::map< SPtr<Node>, SPtr<Node> > replacements;

	replacements[copy.initial_node()] = _initial_node;

	for (const auto& n : copy._nodes) {
		if (!n->is_initial()) {
			SPtr<machina::Node> node(new machina::Node(*n.get()));
			_nodes.insert(node);
			replacements[n] = node;
		}
	}

	for (const auto& n : _nodes) {
		for (const auto& e : n->edges()) {
			e->set_tail(n);
			e->set_head(replacements[e->head()]);
		}
	}
}

Machine::Machine(const Machine& copy)
	: Stateful() // don't copy RDF ID
	, fitness(0.0)
	, _initial_node(new Node(TimeStamp(copy.time().unit(), 0, 0), true))
	, _active_nodes(MAX_ACTIVE_NODES, SPtr<Node>())
	, _time(copy.time())
	, _is_finished(false)
{
	_nodes.insert(_initial_node);
	assign(copy);
}

Machine&
Machine::operator=(const Machine& copy)
{
	if (&copy == this) {
		return *this;
	}

	fitness = copy.fitness;

	_active_nodes  = std::vector< SPtr<Node> >(MAX_ACTIVE_NODES, SPtr<Node>());
	_is_finished   = false;
	_time          = copy._time;
	_pending_learn = SPtr<LearnRequest>();

	_nodes.clear();
	_nodes.insert(_initial_node);
	assign(copy);

	return *this;
}

bool
Machine::operator==(const Machine& rhs) const
{
	return false;
}

void
Machine::merge(const Machine& machine)
{
	for (const auto& m : machine.nodes()) {
		if (m->is_initial()) {
			for (const auto& e : m->edges()) {
				e->set_tail(_initial_node);
				_initial_node->edges().insert(e);
			}
		} else {
			_nodes.insert(m);
		}
	}
}

/** Always returns a node, unless there are none */
SPtr<Node>
Machine::random_node()
{
	if (_nodes.empty()) {
		return SPtr<Node>();
	}

	size_t i = rand() % _nodes.size();

	// FIXME: O(n) worst case :(
	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n,
		     --i) {
		if (i == 0) {
			return *n;
		}
	}

	return SPtr<Node>();
}

/** May return NULL even if edges exist (with low probability) */
SPtr<Edge>
Machine::random_edge()
{
	SPtr<Node> tail = random_node();

	for (size_t i = 0; i < _nodes.size() && tail->edges().empty(); ++i) {
		tail = random_node();
	}

	return tail ? tail->random_edge() : SPtr<Edge>();
}

void
Machine::add_node(SPtr<Node> node)
{
	_nodes.insert(node);
}

void
Machine::remove_node(SPtr<Node> node)
{
	_nodes.erase(node);

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		(*n)->remove_edge_to(node);
	}
}

void
Machine::reset(MIDISink* sink, Raul::TimeStamp time)
{
	if (!_is_finished) {
		for (auto& n : _active_nodes) {
			if (n) {
				n->exit(sink, time);
				n.reset();
			}
		}
	}

	_time        = TimeStamp(_time.unit(), 0, 0);
	_is_finished = false;
}

SPtr<Node>
Machine::earliest_node() const
{
	SPtr<Node> earliest;

	for (const auto& n : _active_nodes) {
		if (n && (!earliest || n->exit_time() < earliest->exit_time())) {
			earliest = n;
		}
	}

	return earliest;
}

bool
Machine::enter_node(Context&               context,
                    SPtr<Node>             node,
                    SPtr<Raul::RingBuffer> updates)
{
	assert(!node->is_active());
	assert(_active_nodes.size() == MAX_ACTIVE_NODES);

	/* FIXME: Would be best to use the MIDI note here as a hash key, at least
	 * while all actions are still MIDI notes... */
	size_t index = (rand() % MAX_ACTIVE_NODES);
	for (size_t i = 0; i < MAX_ACTIVE_NODES; ++i) {
		if (!_active_nodes[index]) {
			node->enter(context.sink(), _time);
			_active_nodes[index] = node;

			write_set(updates,
			          node->id(),
			          URIs::instance().machina_active,
			          context.forge().make(true));
			return true;
		}
		index = (index + 1) % MAX_ACTIVE_NODES;
	}

	// If we get here, ran out of active node spots.  Don't enter node
	return false;
}

void
Machine::exit_node(Context&               context,
                   SPtr<Node>             node,
                   SPtr<Raul::RingBuffer> updates)
{
	// Exit node
	node->exit(context.sink(), _time);

	// Notify UI
	write_set(updates,
	          node->id(),
	          URIs::instance().machina_active,
	          context.forge().make(false));

	// Remove node from _active_nodes
	for (auto& n : _active_nodes) {
		if (n == node) {
			n.reset();
		}
	}

	// Activate successors
	if (node->is_selector()) {
		const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]
		double       range_min   = 0;

		for (const auto& e : node->edges()) {
			if (!e->head()->is_active()
			    && rand_normal > range_min
			    && rand_normal < range_min + e->probability()) {

				enter_node(context, e->head(), updates);
				break;

			} else {
				range_min += e->probability();
			}
		}
	} else {
		for (const auto& e : node->edges()) {
			const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]
			if (rand_normal <= e->probability()) {
				SPtr<Node> head = e->head();

				if (!head->is_active()) {
					enter_node(context, head, updates);
				}
			}
		}
	}
}

uint32_t
Machine::run(Context& context, SPtr<Raul::RingBuffer> updates)
{
	if (_is_finished) {
		return 0;
	}

	const Raul::TimeSlice& time = context.time();

	const TimeStamp end_frames = (time.start_ticks() + time.length_ticks());
	const TimeStamp end_beats  = time.ticks_to_beats(end_frames);

	if (_time.is_zero()) {  // Initial run
		// Exit any active nodes
		for (auto& n : _active_nodes) {
			if (n && n->is_active()) {
				n->exit(context.sink(), _time);
				write_set(updates,
				          n->id(),
				          URIs::instance().machina_active,
				          context.forge().make(false));
			}
			n.reset();
		}

		// Enter initial node
		enter_node(context, _initial_node, updates);

		if (_initial_node->edges().empty()) {  // Nowhere to go, exit
			_is_finished = true;
			return 0;
		}
	}

	while (true) {
		SPtr<Node> earliest = earliest_node();
		if (!earliest) {
			// No more active states, machine is finished
			_is_finished = true;
			break;
		}

		const TimeStamp exit_time = earliest->exit_time();
		if (time.beats_to_ticks(exit_time) < end_frames) {
			// Earliest active state ends this cycle, exit it
			_time = earliest->exit_time();
			exit_node(context, earliest, updates);

		} else {
			// Earliest active state ends in the future, done this cycle
			_time = end_beats;
			break;
		}

	}

	return time.beats_to_ticks(_time).ticks() - time.start_ticks().ticks();
}

void
Machine::learn(SPtr<Raul::Maid> maid, SPtr<Node> node)
{
	_pending_learn = LearnRequest::create(maid, node);
}

void
Machine::write_state(Sord::Model& model)
{
	using namespace Raul;

	model.add_statement(model.base_uri(),
	                    Sord::URI(model.world(), MACHINA_URI_RDF "type"),
	                    Sord::URI(model.world(), MACHINA_NS_Machine));

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		(*n)->write_state(model);

		if ((*n)->is_initial()) {
			model.add_statement(model.base_uri(),
			                    Sord::URI(model.world(), MACHINA_NS_start),
			                    (*n)->rdf_id(model.world()));
		} else {
			model.add_statement(model.base_uri(),
			                    Sord::URI(model.world(), MACHINA_NS_node),
			                    (*n)->rdf_id(model.world()));
		}
	}

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		for (Node::Edges::const_iterator e = (*n)->edges().begin();
		     e != (*n)->edges().end(); ++e) {

			(*e)->write_state(model);

			model.add_statement(model.base_uri(),
			                    Sord::URI(model.world(), MACHINA_NS_arc),
			                    (*e)->rdf_id(model.world()));
		}

	}
}

} // namespace machina
