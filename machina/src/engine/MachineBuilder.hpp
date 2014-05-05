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

#ifndef MACHINA_MACHINEBUILDER_HPP
#define MACHINA_MACHINEBUILDER_HPP

#include <stdint.h>

#include <list>

#include "machina/types.hpp"
#include "raul/TimeStamp.hpp"

namespace machina {

class Machine;
class Node;

class MachineBuilder
{
public:
	MachineBuilder(SPtr<Machine> machine,
	               double        quantization,
	               bool          step);

	void event(Raul::TimeStamp time, size_t size, unsigned char* buf);

	void set_step(bool step) { _step = step; }

	void reset();
	void resolve();

	SPtr<Machine> finish();

private:
	bool is_delay_node(SPtr<Node> node) const;
	void set_node_duration(SPtr<Node> node, Raul::TimeDuration d) const;

	void note_on(Raul::TimeStamp t, size_t ev_size, uint8_t* buf);

	void resolve_note(Raul::TimeStamp t,
	                  size_t          ev_size,
	                  uint8_t*        buf,
	                  SPtr<Node>      resolved);

	SPtr<Node>connect_nodes(SPtr<Machine>   m,
	                        SPtr<Node>      tail,
	                        Raul::TimeStamp tail_end_time,
	                        SPtr<Node>      head,
	                        Raul::TimeStamp head_start_time);

	Raul::TimeStamp default_duration() {
		return _step ? _step_duration : Raul::TimeStamp(_time.unit(), 0, 0);
	}

	typedef std::list<SPtr<Node> > ActiveList;
	ActiveList _active_nodes;

	typedef std::list<std::pair<Raul::TimeStamp, SPtr<Node> > > PolyList;
	PolyList _poly_nodes;

	double          _quantization;
	Raul::TimeStamp _time;
	SPtr<Machine>   _machine;
	SPtr<Node>      _initial_node;
	SPtr<Node>      _connect_node;
	Raul::TimeStamp _connect_node_end_time;
	Raul::TimeStamp _step_duration;
	bool            _step;
};

} // namespace machina

#endif // MACHINA_MACHINEBUILDER_HPP
