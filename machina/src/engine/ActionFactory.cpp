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

#include "ActionFactory.hpp"
#include "MidiAction.hpp"

namespace machina {

SPtr<Action>
ActionFactory::copy(SPtr<Action> copy)
{
	SPtr<MidiAction> ma = dynamic_ptr_cast<MidiAction>(copy);
	if (ma) {
		return SPtr<Action>(new MidiAction(ma->event_size(), ma->event()));
	} else {
		return SPtr<Action>();
	}
}

SPtr<Action>
ActionFactory::note_on(uint8_t note, uint8_t velocity)
{
	uint8_t buf[3];
	buf[0] = 0x90;
	buf[1] = note;
	buf[2] = velocity;

	return SPtr<Action>(new MidiAction(3, buf));
}

SPtr<Action>
ActionFactory::note_off(uint8_t note, uint8_t velocity)
{
	uint8_t buf[3];
	buf[0] = 0x80;
	buf[1] = note;
	buf[2] = velocity;

	return SPtr<Action>(new MidiAction(3, buf));
}

} // namespace Machine
