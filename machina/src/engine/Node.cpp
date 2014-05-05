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

#include <cassert>
#include <iostream>

#include "sord/sordmm.hpp"

#include "machina/Atom.hpp"
#include "machina/URIs.hpp"

#include "ActionFactory.hpp"
#include "Edge.hpp"
#include "Node.hpp"

using namespace Raul;
using namespace std;

namespace machina {

Node::Node(TimeDuration duration, bool initial)
	: _enter_time(duration.unit())
	, _duration(duration)
	, _is_initial(initial)
	, _is_selector(false)
	, _is_active(false)
{}

Node::Node(const Node& copy)
	: Stateful() // don't copy RDF ID
	, _enter_time(copy._enter_time)
	, _duration(copy._duration)
	, _enter_action(ActionFactory::copy(copy._enter_action))
	, _exit_action(ActionFactory::copy(copy._exit_action))
	, _is_initial(copy._is_initial)
	, _is_selector(copy._is_selector)
	, _is_active(false)
{
	for (Edges::const_iterator i = copy._edges.begin(); i != copy._edges.end();
	     ++i) {
		SPtr<Edge> edge(new Edge(*i->get()));
		_edges.insert(edge);
	}
}

static inline bool
action_equals(SPtr<const Action> a, SPtr<const Action> b)
{
	return (a == b) || (a && b && *a.get() == *b.get());
}

bool
Node::operator==(const Node& rhs) const
{
	return _duration == rhs._duration &&
		_is_initial == rhs._is_initial &&
		_is_selector == rhs._is_selector &&
		_is_active == rhs._is_active &&
		action_equals(_enter_action, rhs.enter_action()) &&
		action_equals(_exit_action, rhs.exit_action());
	// TODO: compare edges
}

/** Always returns an edge, unless there are none */
SPtr<Edge>
Node::random_edge()
{
	SPtr<Edge> ret;
	if (_edges.empty()) {
		return ret;
	}

	size_t i = rand() % _edges.size();

	// FIXME: O(n) worst case :(
	for (Edges::const_iterator e = _edges.begin(); e != _edges.end(); ++e,
		     --i) {
		if (i == 0) {
			ret = *e;
			break;
		}
	}

	return ret;
}

void
Node::edges_changed()
{
	if (!_is_selector) {
		return;
	}

	// Normalize edge probabilities if we're a selector
	double prob_sum = 0;

	for (Edges::iterator i = _edges.begin(); i != _edges.end(); ++i) {
		prob_sum += (*i)->probability();
	}

	for (Edges::iterator i = _edges.begin(); i != _edges.end(); ++i) {
		(*i)->set_probability((*i)->probability() / prob_sum);
	}

	_changed = true;
}

void
Node::set_selector(bool yn)
{
	_is_selector = yn;

	if (yn) {
		edges_changed();
	}

	_changed = true;
}

void
Node::set_enter_action(SPtr<Action> action)
{
	_enter_action = action;
	_changed      = true;
}

void
Node::set_exit_action(SPtr<Action> action)
{
	_exit_action = action;
	_changed     = true;
}

void
Node::enter(MIDISink* sink, TimeStamp time)
{
	if (!_is_active) {
		_changed    = true;
		_is_active  = true;
		_enter_time = time;

		if (sink && _enter_action) {
			_enter_action->execute(sink, time);
		}
	}
}

void
Node::exit(MIDISink* sink, TimeStamp time)
{
	if (_is_active) {
		if (sink && _exit_action) {
			_exit_action->execute(sink, time);
		}

		_changed    = true;
		_is_active  = false;
		_enter_time = 0;
	}
}

SPtr<Edge>
Node::edge_to(SPtr<Node> head) const
{
	// TODO: Make logarithmic
	for (Edges::const_iterator i = _edges.begin(); i != _edges.end(); ++i) {
		if ((*i)->head() == head) {
			return *i;
		}
	}
	return SPtr<Edge>();
}

void
Node::add_edge(SPtr<Edge> edge)
{
	assert(edge->tail().lock().get() == this);
	if (edge_to(edge->head())) {
		return;
	}

	_edges.insert(edge);
	edges_changed();
}

void
Node::remove_edge(SPtr<Edge> edge)
{
	_edges.erase(edge);
	edges_changed();
}

bool
Node::connected_to(SPtr<Node> node)
{
	return bool(edge_to(node));
}

SPtr<Edge>
Node::remove_edge_to(SPtr<Node> node)
{
	SPtr<Edge> edge = edge_to(node);
	if (edge) {
		_edges.erase(edge);  // TODO: avoid double search
		edges_changed();
	}
	return edge;
}

void
Node::set(URIInt key, const Atom& value)
{
	if (key == URIs::instance().machina_initial) {
		std::cerr << "error: Attempt to change node initial state" << std::endl;
	} else if (key == URIs::instance().machina_selector) {
		set_selector(value.get<int32_t>());
	}
}

void
Node::write_state(Sord::Model& model)
{
	using namespace Raul;

	const Sord::Node& rdf_id = this->rdf_id(model.world());

	if (_is_selector)
		model.add_statement(
			rdf_id,
			Sord::URI(model.world(), MACHINA_URI_RDF "type"),
			Sord::URI(model.world(), MACHINA_NS_SelectorNode));
	else
		model.add_statement(
			rdf_id,
			Sord::URI(model.world(), MACHINA_URI_RDF "type"),
			Sord::URI(model.world(), MACHINA_NS_Node));

	model.add_statement(
		rdf_id,
		Sord::URI(model.world(), MACHINA_NS_duration),
		Sord::Literal::decimal(model.world(), _duration.to_double(), 7));

	if (_enter_action) {
		_enter_action->write_state(model);
		model.add_statement(rdf_id,
		                    Sord::URI(model.world(), MACHINA_NS_onEnter),
		                    _enter_action->rdf_id(model.world()));
	}

	if (_exit_action) {
		_exit_action->write_state(model);
		model.add_statement(rdf_id,
		                    Sord::URI(model.world(), MACHINA_NS_onExit),
		                    _exit_action->rdf_id(model.world()));
	}
}

} // namespace machina
