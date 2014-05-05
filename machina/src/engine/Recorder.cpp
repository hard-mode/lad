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

#include <ios>
#include <iostream>

#include "MachineBuilder.hpp"
#include "Recorder.hpp"

using namespace std;
using namespace Raul;

namespace machina {

Recorder::Recorder(Forge&   forge,
                   size_t   buffer_size,
                   TimeUnit unit,
                   double   q,
                   bool     step)
	: _forge(forge)
	, _unit(unit)
	, _record_buffer(buffer_size)
	, _builder(new MachineBuilder(SPtr<Machine>(new Machine(unit)), q, step))
{}

void
Recorder::_whipped()
{
	TimeStamp     t(_unit);
	size_t        size;
	unsigned char buf[4];

	while (true) {
		bool success = _record_buffer.read(sizeof(TimeStamp), (uint8_t*)&t);
		if (success) {
			success = _record_buffer.read(sizeof(size_t), (uint8_t*)&size);
		}
		if (success) {
			success = _record_buffer.read(size, buf);
		}
		if (success) {
			_builder->event(t, size, buf);
		} else {
			break;
		}
	}
}

SPtr<Machine>
Recorder::finish()
{
	SPtr<Machine> machine = _builder->finish();
	_builder.reset();
	return machine;
}

}
