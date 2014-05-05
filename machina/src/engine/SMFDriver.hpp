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

#ifndef MACHINA_SMFDRIVER_HPP
#define MACHINA_SMFDRIVER_HPP

#include <glibmm/ustring.h>

#include "machina/Driver.hpp"
#include "machina/types.hpp"

#include "MachineBuilder.hpp"
#include "SMFReader.hpp"
#include "SMFWriter.hpp"

namespace machina {

class Node;
class Machine;

class SMFDriver : public Driver
{
public:
	SMFDriver(Forge& forge, Raul::TimeUnit unit);

	SPtr<Machine> learn(const std::string& filename,
	                    double             q,
	                    Raul::TimeDuration max_duration);

	SPtr<Machine> learn(const std::string& filename,
	                    unsigned           track,
	                    double             q,
	                    Raul::TimeDuration max_duration);

	void run(SPtr<Machine> machine, Raul::TimeStamp max_time);

	void write_event(Raul::TimeStamp      time,
	                 size_t               ev_size,
	                 const unsigned char* ev) throw (std::logic_error)
	{ _writer->write_event(time, ev_size, ev); }

	SPtr<SMFWriter> writer() { return _writer; }

private:
	SPtr<SMFWriter> _writer;

	void learn_track(SPtr<MachineBuilder> builder,
	                 SMFReader&           reader,
	                 unsigned             track,
	                 double               q,
	                 Raul::TimeDuration   max_duration);
};

} // namespace machina

#endif // MACHINA_SMFDRIVER_HPP
