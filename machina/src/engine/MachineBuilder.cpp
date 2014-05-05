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

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "machina/Machine.hpp"
#include "machina/types.hpp"

#include "Edge.hpp"
#include "MachineBuilder.hpp"
#include "MidiAction.hpp"
#include "Node.hpp"
#include "quantize.hpp"

using namespace std;
using namespace Raul;

namespace machina {

MachineBuilder::MachineBuilder(SPtr<Machine> machine, double q, bool step)
	: _quantization(q)
	, _time(machine->time().unit()) // = 0
	, _machine(machine)
	, _initial_node(machine->initial_node()) // duration 0
	, _connect_node(_initial_node)
	, _connect_node_end_time(_time) // = 0
	, _step_duration(_time.unit(), q)
	, _step(step)
{}

void
MachineBuilder::reset()
{
	_time                  = TimeStamp(_machine->time().unit()); // = 0
	_connect_node          = _initial_node;
	_connect_node_end_time = _time; // = 0
}

bool
MachineBuilder::is_delay_node(SPtr<Node> node) const
{
	return node != _initial_node &&
		!node->enter_action() &&
		!node->exit_action();
}

/** Set the duration of a node, with quantization.
 */
void
MachineBuilder::set_node_duration(SPtr<Node>         node,
                                  Raul::TimeDuration d) const
{
	if (_step) {
		node->set_duration(_step_duration);
		return;
	}

	Raul::TimeStamp q_dur = quantize(TimeStamp(d.unit(), _quantization), d);

	// Never quantize a note to duration 0
	if (q_dur.is_zero() && (node->enter_action() || node->exit_action())) {
		q_dur = _quantization; // Round up
	}
	node->set_duration(q_dur);
}

/** Connect two nodes, inserting a delay node between them if necessary.
 *
 * If a delay node is added to the machine, it is returned.
 */
SPtr<Node>
MachineBuilder::connect_nodes(SPtr<Machine>   m,
                              SPtr<Node>      tail,
                              Raul::TimeStamp tail_end_time,
                              SPtr<Node>      head,
                              Raul::TimeStamp head_start_time)
{
	SPtr<Node> delay_node;
	if (tail == head) {
		return delay_node;
	}

	if (is_delay_node(tail) && tail->edges().empty()) {
		// Tail is a delay node, just accumulate the time difference into it
		set_node_duration(tail,
		                  tail->duration() + head_start_time - tail_end_time);
		tail->add_edge(SPtr<Edge>(new Edge(tail, head)));
	} else if (_step || (head_start_time == tail_end_time)) {
		// Connect directly
		tail->add_edge(SPtr<Edge>(new Edge(tail, head)));
	} else {
		// Need to actually create a delay node
		delay_node = SPtr<Node>(new Node(head_start_time - tail_end_time));
		tail->add_edge(SPtr<Edge>(new Edge(tail, delay_node)));
		delay_node->add_edge(SPtr<Edge>(new Edge(delay_node, head)));
		m->add_node(delay_node);
	}

	return delay_node;
}

void
MachineBuilder::note_on(Raul::TimeStamp t, size_t ev_size, uint8_t* buf)
{
	SPtr<Node> node;
	if (_step && _poly_nodes.empty() && is_delay_node(_connect_node)) {
		/* Stepping and the connect node is the merge node after a polyphonic
		   group.  Re-use it to avoid creating delay nodes in step mode. */
		node = _connect_node;
		node->set_duration(_step_duration);
	} else {
		node = SPtr<Node>(new Node(default_duration()));
	}

	node->set_enter_action(SPtr<Action>(new MidiAction(ev_size, buf)));

	if (_step && _poly_nodes.empty()) {
		t = _time = _time + _step_duration;  // Advance time one step
	}

	SPtr<Node> this_connect_node;
	Raul::TimeStamp this_connect_node_end_time(t.unit());

	/* If currently polyphonic, use a poly node with no successors as connect
	   node, for more sensible patterns like what a human would build. */
	if (!_poly_nodes.empty()) {
		for (PolyList::iterator j = _poly_nodes.begin();
		     j != _poly_nodes.end(); ++j) {
			if (j->second->edges().empty()) {
				this_connect_node          = j->second;
				this_connect_node_end_time = j->first + j->second->duration();
				break;
			}
		}
	}

	/* Currently monophonic, or didn't find a poly node, so use _connect_node
	   which is maintained below on note off events. */
	if (!this_connect_node) {
		this_connect_node          = _connect_node;
		this_connect_node_end_time = _connect_node_end_time;
	}

	SPtr<Node> delay_node = connect_nodes(
		_machine, this_connect_node, this_connect_node_end_time, node, t);

	if (delay_node) {
		_connect_node          = delay_node;
		_connect_node_end_time = t;
	}

	node->enter(NULL, t);
	_active_nodes.push_back(node);
}

void
MachineBuilder::resolve_note(Raul::TimeStamp time,
                             size_t          ev_size,
                             uint8_t*        buf,
                             SPtr<Node>      resolved)
{
	resolved->set_exit_action(SPtr<Action>(new MidiAction(ev_size, buf)));

	if (_active_nodes.size() == 1) {
		if (_step) {
			time = _time = _time + _step_duration;
		}

		// Last active note
		_connect_node_end_time = time;

		if (!_poly_nodes.empty()) {
			// Finish a polyphonic section
			_connect_node = SPtr<Node>(new Node(TimeStamp(_time.unit(), 0, 0)));
			_machine->add_node(_connect_node);

			connect_nodes(_machine, resolved, time, _connect_node, time);

			for (PolyList::iterator j = _poly_nodes.begin();
			     j != _poly_nodes.end(); ++j) {
				_machine->add_node(j->second);
				if (j->second->edges().empty()) {
					connect_nodes(_machine, j->second,
					              j->first + j->second->duration(),
					              _connect_node, time);
				}
			}
			_poly_nodes.clear();

			_machine->add_node(resolved);

		} else {
			// Just monophonic
			if (is_delay_node(_connect_node)
			    && _connect_node->duration().is_zero()
			    && (_connect_node->edges().size() == 1)
			    && ((*_connect_node->edges().begin())->head() == resolved)) {
				// Trim useless delay node if possible (after poly sections)

				_connect_node->edges().clear();
				_connect_node->set_enter_action(resolved->enter_action());
				_connect_node->set_exit_action(resolved->exit_action());
				resolved->set_enter_action(SPtr<Action>());
				resolved->set_exit_action(SPtr<Action>());
				set_node_duration(_connect_node, resolved->duration());
				resolved = _connect_node;
				_machine->add_node(_connect_node);

			} else {
				_connect_node = resolved;
				_machine->add_node(resolved);
			}
		}

	} else {
		// Polyphonic, add this node to poly list
		_poly_nodes.push_back(make_pair(resolved->enter_time(), resolved));
		_connect_node          = resolved;
		_connect_node_end_time = _time;
	}

	if (resolved->is_active()) {
		resolved->exit(NULL, _time);
	}
}

void
MachineBuilder::event(Raul::TimeStamp time,
                      size_t          ev_size,
                      uint8_t*        buf)
{
	if (ev_size == 0) {
		return;
	}

	if (!_step) {
		_time = time;
	}

	if ((buf[0] & 0xF0) == LV2_MIDI_MSG_NOTE_ON) {
		note_on(time, ev_size, buf);
	} else if ((buf[0] & 0xF0) == LV2_MIDI_MSG_NOTE_OFF) {
		for (ActiveList::iterator i = _active_nodes.begin();
		     i != _active_nodes.end(); ++i) {
			SPtr<MidiAction> action = dynamic_ptr_cast<MidiAction>(
				(*i)->enter_action());
			if (!action) {
				continue;
			}

			const size_t   ev_size = action->event_size();
			const uint8_t* ev      = action->event();

			if ((ev[0] & 0xF0) == LV2_MIDI_MSG_NOTE_ON &&
			    (ev[0] & 0x0F) == (buf[0] & 0x0F) &&
			    ev[1] == buf[1]) {
				// Same channel and note as on event
				resolve_note(time, ev_size, buf, *i);
				_active_nodes.erase(i);
				break;
			}
		}
	}
}

/** Finish the constructed machine and prepare it for use.
 * Resolve any stuck notes, quantize, etc.
 */
void
MachineBuilder::resolve()
{
	// Resolve stuck notes
	if (!_active_nodes.empty()) {
		for (list<SPtr<Node> >::iterator i = _active_nodes.begin();
		     i != _active_nodes.end(); ++i) {
			cerr << "WARNING: Resolving stuck note." << endl;
			SPtr<MidiAction> action = dynamic_ptr_cast<MidiAction>(
				(*i)->enter_action());
			if (!action) {
				continue;
			}

			const size_t   ev_size = action->event_size();
			const uint8_t* ev      = action->event();
			if (ev_size == 3 && (ev[0] & 0xF0) == LV2_MIDI_MSG_NOTE_ON) {
				uint8_t st((LV2_MIDI_MSG_NOTE_OFF & 0xF0) | (ev[0] & 0x0F));
				const uint8_t note_off[3] = { st, ev[1], 0x40 };
				(*i)->set_exit_action(
					SPtr<Action>(new MidiAction(3, note_off)));
				set_node_duration((*i), _time - (*i)->enter_time());
				(*i)->exit(NULL, _time);
				_machine->add_node((*i));
			}
		}
		_active_nodes.clear();
	}

	// Add initial node if necessary
	if (!_machine->nodes().empty()) {
		_machine->add_node(_initial_node);
	}
}

SPtr<Machine>
MachineBuilder::finish()
{
	resolve();

	return _machine;
}

} // namespace machina
