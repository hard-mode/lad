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

#ifndef MACHINA_EVOLVER_HPP
#define MACHINA_EVOLVER_HPP

#include "eugene/GA.hpp"
#include "eugene/Random.hpp"
#include "machina/types.hpp"
#include "raul/Thread.hpp"
#include "raul/TimeStamp.hpp"

#include "Machine.hpp"
#include "Schrodinbit.hpp"

namespace eugene {
template<typename G> class HybridMutation;
}

namespace machina {

class Problem;

class Evolver : public Raul::Thread
{
public:
	Evolver(Raul::TimeUnit     unit,
	        const std::string& target_midi,
	        SPtr<Machine>      seed);

	void seed(SPtr<Machine> parent);
	bool improvement() { return _improvement; }

	const Machine& best() { return _ga->best(); }

	typedef eugene::GA<Machine> MachinaGA;

private:
	void _run();

	eugene::Random  _rng;
	SPtr<MachinaGA> _ga;
	SPtr<Problem>   _problem;
	float           _seed_fitness;
	Schrodinbit     _improvement;
};

} // namespace machina

#endif // MACHINA_EVOLVER_HPP
