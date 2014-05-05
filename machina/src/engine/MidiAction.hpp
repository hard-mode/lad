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

#ifndef MACHINA_MIDIACTION_HPP
#define MACHINA_MIDIACTION_HPP

#include <atomic>

#include "raul/TimeSlice.hpp"

#include "machina/types.hpp"

#include "Action.hpp"

namespace machina {

class MIDISink;

class MidiAction : public Action
{
public:
	~MidiAction();

	MidiAction(size_t               size,
	           const unsigned char* event);

	size_t event_size() { return _size; }
	byte*  event()      { return _event.load(); }

	bool set_event(size_t size, const byte* event);

	void execute(MIDISink* sink, Raul::TimeStamp time);

	virtual void write_state(Sord::Model& model);

private:
	size_t             _size;
	const size_t       _max_size;
	std::atomic<byte*> _event;
};

} // namespace machina

#endif // MACHINA_MIDIACTION_HPP
