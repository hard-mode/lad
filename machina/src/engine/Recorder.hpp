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

#ifndef MACHINA_RECORDER_HPP
#define MACHINA_RECORDER_HPP

#include "raul/RingBuffer.hpp"

#include "machina/Machine.hpp"
#include "machina/types.hpp"

#include "Slave.hpp"

namespace machina {

class MachineBuilder;

class Recorder
	: public Slave
{
public:
	Recorder(Forge&   forge,
	         size_t   buffer_size,
	         TimeUnit unit,
	         double   q,
	         bool     step);

	inline void write(Raul::TimeStamp time, size_t size,
	                  const unsigned char* buf) {
		if (_record_buffer.write_space() <
		    (sizeof(TimeStamp) + sizeof(size_t) + size)) {
			std::cerr << "Record buffer overflow" << std::endl;
			return;
		} else {
			_record_buffer.write(sizeof(TimeStamp), (uint8_t*)&time);
			_record_buffer.write(sizeof(size_t), (uint8_t*)&size);
			_record_buffer.write(size, buf);
		}
	}

	SPtr<MachineBuilder> builder() { return _builder; }

	SPtr<Machine> finish();

private:
	virtual void _whipped();

	Forge&               _forge;
	TimeUnit             _unit;
	Raul::RingBuffer     _record_buffer;
	SPtr<MachineBuilder> _builder;
};

} // namespace machina

#endif // MACHINA_RECORDER_HPP
