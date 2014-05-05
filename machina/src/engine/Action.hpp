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

#ifndef MACHINA_ACTION_HPP
#define MACHINA_ACTION_HPP

#include <string>
#include <iostream>

#include "raul/Maid.hpp"
#include "raul/TimeSlice.hpp"

#include "machina/types.hpp"

#include "MIDISink.hpp"
#include "Stateful.hpp"

namespace machina {

/** An Action, executed on entering or exiting of a state.
 */
struct Action
		: public Raul::Maid::Manageable
		, public Stateful
{
	bool operator==(const Action& rhs) const { return false; }

	virtual void execute(MIDISink* sink, Raul::TimeStamp time) = 0;

	virtual void write_state(Sord::Model& model) {}
};

class PrintAction : public Action
{
public:
	explicit PrintAction(const std::string& msg) : _msg(msg) {}

	void execute(MIDISink* sink, Raul::TimeStamp time)
	{ std::cout << "t=" << time << ": " << _msg << std::endl; }

private:
	std::string _msg;
};

} // namespace machina

#endif // MACHINA_ACTION_HPP
