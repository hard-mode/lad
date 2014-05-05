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

#include <iostream>
#include <cstdlib>

#include "machina/Machine.hpp"
#include "machina/Mutation.hpp"

#include "ActionFactory.hpp"
#include "Edge.hpp"
#include "MidiAction.hpp"

using namespace std;

namespace machina {
namespace Mutation {

void
Compress::mutate(Random& rng, Machine& machine)
{
	//cout << "COMPRESS" << endl;

	// Trim disconnected nodes
	for (Machine::Nodes::iterator i = machine.nodes().begin();
	     i != machine.nodes().end(); ) {
		Machine::Nodes::iterator next = i;
		++next;

		if ((*i)->edges().empty()) {
			machine.remove_node(*i);
		}

		i = next;
	}
}

void
AddNode::mutate(Random& rng, Machine& machine)
{
	//cout << "ADD NODE" << endl;

	// Create random node
	SPtr<Node> node(new Node(machine.time().unit()));
	node->set_selector(true);

	SPtr<Node> note_node = machine.random_node();
	if (!note_node) {
		return;
	}

	uint8_t note = rand() % 128;

	SPtr<MidiAction> enter_action = dynamic_ptr_cast<MidiAction>(
	        note_node->enter_action());
	if (enter_action) {
		note = enter_action->event()[1];
	}

	node->set_enter_action(ActionFactory::note_on(note));
	node->set_exit_action(ActionFactory::note_off(note));
	machine.add_node(node);

	// Insert after some node
	SPtr<Node> tail = machine.random_node();
	if (tail && (tail != node) /* && !node->connected_to(tail)*/) {
		tail->add_edge(SPtr<Edge>(new Edge(tail, node)));
	}

	// Insert before some other node
	SPtr<Node> head = machine.random_node();
	if (head && (head != node) /* && !head->connected_to(node)*/) {
		node->add_edge(SPtr<Edge>(new Edge(node, head)));
	}
}

void
RemoveNode::mutate(Random& rng, Machine& machine)
{
	//cout << "REMOVE NODE" << endl;

	SPtr<Node> node = machine.random_node();
	if (node && !node->is_initial()) {
		machine.remove_node(node);
	}
}

void
AdjustNode::mutate(Random& rng, Machine& machine)
{
	//cout << "ADJUST NODE" << endl;

	SPtr<Node> node = machine.random_node();
	if (node) {
		SPtr<MidiAction> enter_action = dynamic_ptr_cast<MidiAction>(
		        node->enter_action());
		SPtr<MidiAction> exit_action  = dynamic_ptr_cast<MidiAction>(
		        node->exit_action());
		if (enter_action && exit_action) {
			const uint8_t note = rand() % 128;
			enter_action->event()[1] = note;
			exit_action->event()[1]  = note;
		}
		node->set_changed();
	}
}

void
SwapNodes::mutate(Random& rng, Machine& machine)
{
	//cout << "SWAP NODE" << endl;

	if (machine.nodes().size() <= 1) {
		return;
	}

	SPtr<Node> a = machine.random_node();
	SPtr<Node> b = machine.random_node();
	while (b == a) {
		b = machine.random_node();
	}

	SPtr<MidiAction> a_enter = dynamic_ptr_cast<MidiAction>(a->enter_action());
	SPtr<MidiAction> a_exit  = dynamic_ptr_cast<MidiAction>(a->exit_action());
	SPtr<MidiAction> b_enter = dynamic_ptr_cast<MidiAction>(b->enter_action());
	SPtr<MidiAction> b_exit  = dynamic_ptr_cast<MidiAction>(b->exit_action());

	uint8_t note_a = a_enter->event()[1];
	uint8_t note_b = b_enter->event()[1];

	a_enter->event()[1] = note_b;
	a_exit->event()[1]  = note_b;
	b_enter->event()[1] = note_a;
	b_exit->event()[1]  = note_a;
}

void
AddEdge::mutate(Random& rng, Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SPtr<Node> tail = machine.random_node();
	SPtr<Node> head = machine.random_node();

	if (tail && head && tail != head) {
		// && !tail->connected_to(head) && !head->connected_to(tail)
		SPtr<Edge> edge(new Edge(tail, head));
		edge->set_probability(rand() / (float)RAND_MAX);
		tail->add_edge(SPtr<Edge>(new Edge(tail, head)));
	}
}

void
RemoveEdge::mutate(Random& rng, Machine& machine)
{
	//cout << "REMOVE EDGE" << endl;

	SPtr<Node> tail = machine.random_node();
	if (tail && !(tail->is_initial() && tail->edges().size() == 1)) {
		tail->remove_edge(tail->random_edge());
	}
}

void
AdjustEdge::mutate(Random& rng, Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SPtr<Edge> edge = machine.random_edge();
	if (edge) {
		edge->set_probability(rand() / (float)RAND_MAX);
		edge->tail().lock()->edges_changed();
	}
}

} // namespace Mutation
} // namespace machina
