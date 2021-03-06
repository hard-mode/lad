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

#include <iostream>

#include "eugene/HybridMutation.hpp"
#include "eugene/Mutation.hpp"
#include "eugene/TournamentSelection.hpp"

#include "machina/Evolver.hpp"
#include "machina/Mutation.hpp"

#include "Problem.hpp"

using namespace std;
using namespace eugene;

namespace machina {

Evolver::Evolver(TimeUnit      unit,
                 const string& target_midi,
                 SPtr<Machine> seed)
	: _problem(new Problem(unit, target_midi, seed))
	, _seed_fitness(-FLT_MAX)
{
	SPtr<eugene::HybridMutation<Machine> > m(new HybridMutation<Machine>());

	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::Compress()));
	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::AddNode()));
	//m->append_mutation(1/6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
	//		new Mutation::RemoveNode()));
	//m->append_mutation(1/6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
	//		new Mutation::AdjustNode()));
	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::SwapNodes()));
	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::AddEdge()));
	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::RemoveEdge()));
	m->append_mutation(1 / 6.0f, std::shared_ptr< eugene::Mutation<Machine> >(
		                   new Mutation::AdjustEdge()));

	std::shared_ptr< Selection<Machine> > s(
		new TournamentSelection<Machine>(*_problem.get(), 3, 0.8));
	std::shared_ptr< Crossover<Machine> > crossover;
	size_t gene_length = 20; // FIXME
	Problem::Population pop;
	_ga = SPtr<MachinaGA>(
		new MachinaGA(_rng,
		              _problem,
		              s,
		              crossover,
		              m,
		              pop,
		              gene_length,
		              20,
		              2,
		              1.0,
		              0.0));
}

void
Evolver::seed(SPtr<Machine> parent)
{
	/*_best = SPtr<Machine>(new Machine(*parent.get()));
	  _best_fitness = _problem->fitness(*_best.get());*/
	_problem->seed(parent);
	_seed_fitness = _problem->evaluate(*parent.get());
}

void
Evolver::_run()
{
	float old_best = _ga->best_fitness();

	//cout << "ORIGINAL BEST: " << _ga->best_fitness() << endl;

	_improvement = true;

	while (!_exit_flag) {
		//cout << "{" << endl;
		_problem->clear_fitness_cache();
		_ga->iteration();

		float new_best = _ga->best_fitness();

		/*cout << _problem->fitness_less(old_best, *_ga->best().get()) << endl;
		  cout << "best: " << _ga->best().get() << endl;
		  cout << "best fitness: " << _problem->fitness(*_ga->best().get()) << endl;
		  cout << "old best: " << old_best << endl;
		  cout << "new best: " << new_best << endl;*/
		cout << "generation best: " << new_best << endl;

		if (_problem->fitness_less_than(old_best, new_best)) {
			_improvement = true;
			old_best     = new_best;
			cout << "*** NEW BEST: " << new_best << endl;
		}

		//cout << "}" << endl;
	}
}

} // namespace machina
