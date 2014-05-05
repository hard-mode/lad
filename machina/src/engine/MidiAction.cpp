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
#include "machina/Atom.hpp"
#include "machina/URIs.hpp"
#include "machina/types.hpp"

#include "MIDISink.hpp"
#include "MidiAction.hpp"

namespace machina {

/** Create a MIDI action.
 *
 * Creating a NULL MidiAction is okay, pass event=NULL and the action will
 * simply do nothing until a set_event (for MIDI learning).
 */
MidiAction::MidiAction(size_t      size,
                       const byte* event)
	: _size(0)
	, _max_size(size)
{
	_event = new byte[_max_size];
	set_event(size, event);
}

MidiAction::~MidiAction()
{
	delete[] _event.load();
}

bool
MidiAction::set_event(size_t size, const byte* new_event)
{
	byte* const event = _event.load();
	if (size <= _max_size) {
		_event = NULL;
		if (size > 0 && new_event) {
			memcpy(event, new_event, size);
		}
		_size  = size;
		_event = event;
		return true;
	} else {
		return false;
	}
}

void
MidiAction::execute(MIDISink* sink, Raul::TimeStamp time)
{
	const byte* const ev = _event.load();
	if (ev && sink) {
		sink->write_event(time, _size, ev);
	}
}

void
MidiAction::write_state(Sord::Model& model)
{
	const uint8_t* ev   = event();
	const uint8_t  type = (ev[0] & 0xF0);
	if (type == LV2_MIDI_MSG_NOTE_ON) {
		model.add_statement(
			rdf_id(model.world()),
			Sord::URI(model.world(), MACHINA_URI_RDF "type"),
			Sord::URI(model.world(), LV2_MIDI__NoteOn));
	} else if (type == LV2_MIDI_MSG_NOTE_OFF) {
		model.add_statement(
			rdf_id(model.world()),
			Sord::URI(model.world(), MACHINA_URI_RDF "type"),
			Sord::URI(model.world(), LV2_MIDI__NoteOff));
	} else {
		std::cerr << "warning: Unable to serialise MIDI event" << std::endl;
	}

	model.add_statement(
		rdf_id(model.world()),
		Sord::URI(model.world(), LV2_MIDI__noteNumber),
		Sord::Literal::integer(model.world(), (int)ev[1]));
	if (ev[2] != 64) {
		model.add_statement(
			rdf_id(model.world()),
			Sord::URI(model.world(), LV2_MIDI__velocity),
			Sord::Literal::integer(model.world(), (int)ev[2]));
	}
}

} // namespace machina
