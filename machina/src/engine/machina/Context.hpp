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

#ifndef MACHINA_CONTEXT_HPP
#define MACHINA_CONTEXT_HPP

#include "machina/Atom.hpp"
#include "machina/types.hpp"
#include "raul/TimeSlice.hpp"

namespace machina {

class MIDISink;

class Context
{
public:
	Context(Forge& forge, uint32_t rate, uint32_t ppqn, double bpm)
		: _forge(forge)
		, _time(rate, ppqn, bpm)
	{}

	void set_sink(MIDISink* sink) { _sink = sink; }

	Forge&                 forge()      { return _forge; }
	const Raul::TimeSlice& time() const { return _time; }
	Raul::TimeSlice&       time()       { return _time; }
	MIDISink*              sink()       { return _sink; }

private:
	Forge&          _forge;
	Raul::TimeSlice _time;
	MIDISink*       _sink;
};

} // namespace machina

#endif // MACHINA_CONTEXT_HPP
