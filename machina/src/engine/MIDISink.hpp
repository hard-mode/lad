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

#ifndef MACHINA_MIDI_SINK_HPP
#define MACHINA_MIDI_SINK_HPP

#include "raul/Deletable.hpp"
#include "raul/TimeStamp.hpp"

namespace machina {

/** Pure virtual base for anything you can write MIDI to. */
class MIDISink
	: public Raul::Deletable
{
public:
	virtual void write_event(Raul::TimeStamp time,
	                         size_t          ev_size,
	                         const uint8_t*  ev) = 0;
};

} // namespace machina

#endif // MACHINA_MIDI_SINK_HPP
