/*
  This file is part of Machina.
  Copyright 2007-2014 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_CONTROLLER_HPP
#define MACHINA_CONTROLLER_HPP

#include <stdint.h>

#include <set>

#include "raul/RingBuffer.hpp"
#include "raul/Maid.hpp"

#include "machina/Model.hpp"
#include "machina/URIs.hpp"
#include "machina/types.hpp"

#include "Stateful.hpp"

namespace Raul {
class Atom;
}

namespace machina {

class Engine;
class Machine;

class Controller
{
public:
	Controller(SPtr<Engine> engine, Model& model);

	uint64_t create(const Properties& properties);
	uint64_t connect(uint64_t tail_id, uint64_t head_id);

	void set_property(uint64_t object_id, URIInt key, const Atom& value);

	void learn(SPtr<Raul::Maid> maid, uint64_t node_id);
	void disconnect(uint64_t tail_id, uint64_t head_id);
	void erase(uint64_t id);

	void announce(SPtr<Machine> machine);

	void process_updates();

private:
	SPtr<Stateful> find(uint64_t id);

	struct StatefulComparator {
		inline bool operator()(SPtr<Stateful> a, SPtr<Stateful> b) const {
			return a->id() < b->id();
		}
	};

	typedef std::set<SPtr<Stateful>, StatefulComparator> Objects;
	Objects _objects;

	SPtr<Engine> _engine;
	Model&       _model;

	SPtr<Raul::RingBuffer> _updates;
};

}

#endif // MACHINA_CONTROLLER_HPP
