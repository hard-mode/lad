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

#ifndef MACHINA_MACHINE_MUTATION_HPP
#define MACHINA_MACHINE_MUTATION_HPP

#include "machina_config.h"

#ifdef HAVE_EUGENE
#    include "eugene/Mutation.hpp"
#    define SUPER : public eugene::Mutation<Machine>
#else
#    define SUPER : public Mutation
#endif

namespace machina {

#ifdef HAVE_EUGENE
typedef eugene::Random Random;
#else
struct Random {};
#endif

class Machine;

namespace Mutation {

struct Mutation {
	virtual ~Mutation() {}

	virtual void mutate(Random& rng, Machine& machine) = 0;
};

struct Compress   SUPER { void mutate(Random& rng, Machine& machine); };
struct AddNode    SUPER { void mutate(Random& rng, Machine& machine); };
struct RemoveNode SUPER { void mutate(Random& rng, Machine& machine); };
struct AdjustNode SUPER { void mutate(Random& rng, Machine& machine); };
struct SwapNodes  SUPER { void mutate(Random& rng, Machine& machine); };
struct AddEdge    SUPER { void mutate(Random& rng, Machine& machine); };
struct RemoveEdge SUPER { void mutate(Random& rng, Machine& machine); };
struct AdjustEdge SUPER { void mutate(Random& rng, Machine& machine); };

} // namespace Mutation

} // namespace machina

#endif // MACHINA_MACHINE_MUTATION_HPP
