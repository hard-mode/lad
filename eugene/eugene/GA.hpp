/* This file is part of Eugene
 * Copyright 2007-2012 David Robillard <http://drobilla.net>
 *
 * Eugene is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Eugene is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Eugene.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EUGENE_GA_HPP
#define EUGENE_GA_HPP

#include <float.h>

#include <atomic>
#include <cassert>
#include <list>
#include <memory>
#include <ostream>
#include <utility>

#include "eugene/Crossover.hpp"
#include "eugene/Mutation.hpp"
#include "eugene/Problem.hpp"
#include "eugene/Random.hpp"
#include "eugene/Selection.hpp"

namespace eugene {

template<typename G>
class GA
{
public:
	GA(Random&                          rng,
	   std::shared_ptr< Problem<G> >    problem,
	   std::shared_ptr< Selection<G> >  selection,
	   std::shared_ptr< Crossover<G> >  crossover,
	   std::shared_ptr< Mutation<G> >   mutation,
	   typename Problem<G>::Population& population,
	   size_t                           gene_length,
	   size_t                           population_size,
	   size_t                           num_elites,
	   float                            mutation_probability,
	   float                            crossover_probability);

	~GA() {}

	const G& best() const { return _best; }

	// FIXME: not really atomic
	void set_mutation_probability(float p)  { _mutation_probability = p; }
	void set_crossover_probability(float p) { _crossover_probability = p; }
	void set_num_elites(size_t n)           { _num_elites = n; }

	void iteration();

	float best_fitness() const { return _problem->evaluate(_best); }

	bool fitness_less_than(float a, float b) const
	{ return _problem->fitness_less_than(a, b); }

	inline int     generation()      const { return _generation; }
	inline size_t  population_size() const { return _population->size(); }
	inline bool    optimum_known()   const { return _problem->optimum_known(); }
	inline int32_t optimum()         const { return _problem->optimum(); }
	inline int     evaluations()     const { return _selection->evaluations(); }

	std::shared_ptr< Problem<G> >   problem()   const { return _problem; }
	std::shared_ptr< Selection<G> > selection() const { return _selection; }
	std::shared_ptr< Crossover<G> > crossover() const { return _crossover; }
	std::shared_ptr< Mutation<G> >  mutation()  const { return _mutation; }

	const typename Problem<G>::Population & population() const {
		return _population;
	}

protected:

	typedef std::pair<typename Problem<G>::Population::const_iterator,
		typename Problem<G>::Population::const_iterator>
	GenePair;

	typedef std::list<G> Elites;

	GenePair select_parents(const float total) const;
	void find_elites(typename Problem<G>::Population & population);

	Random&                          _rng;
	std::shared_ptr< Problem<G> >    _problem;
	std::shared_ptr< Selection<G> >  _selection;
	std::shared_ptr< Crossover<G> >  _crossover;
	std::shared_ptr< Mutation<G> >   _mutation;
	typename Problem<G>::Population  _population;
	G                                _best;
	std::atomic<unsigned>            _generation;
	Elites                           _elites;
	size_t                           _gene_length;
	size_t                           _num_elites;
	float                            _mutation_probability;
	float                            _crossover_probability;
	float                            _elite_threshold;
};

template<typename G>
GA<G>::GA(Random&                          rng,
          std::shared_ptr< Problem<G> >    problem,
          std::shared_ptr< Selection<G> >  selection,
          std::shared_ptr< Crossover<G> >  crossover,
          std::shared_ptr< Mutation<G> >   mutation,
          typename Problem<G>::Population& population,
          size_t                           gene_length,
          size_t                           population_size,
          size_t                           num_elites,
          float                            mutation_probability,
          float                            crossover_probability)
	: _rng(rng)
	, _problem(problem)
	, _selection(selection)
	, _crossover(crossover)
	, _mutation(mutation)
	, _population(population)
	, _best(population.front())
	, _generation(0)
	, _gene_length(gene_length)
	, _num_elites(num_elites)
	, _mutation_probability(mutation_probability)
	, _crossover_probability(crossover_probability)
	, _elite_threshold(FLT_MAX)
{
	assert(gene_length > 0);
	for (auto& p : _population) {
		p.fitness = _problem->evaluate(p);
	}
	_best = _population.front();
	find_elites(_population);
	assert(_elites.size() == num_elites);
}

template<typename G>
void
GA<G>::find_elites(typename Problem<G>::Population& population)
{
	typename Problem<G>::FitnessGreaterThan cmp(*_problem.get());

	// Need more elites, select fittest members from population
	if (_elites.size() < _num_elites) {
		sort(population.begin(), population.end(), cmp);

		for (typename Problem<G>::Population::const_iterator i =
		         population.begin();
		     i != population.end() && _elites.size() < _num_elites; ++i) {

			// FIXME
#if 0
			if (_elites.empty() || cmp(_elites.back(), *i)
			    /*&& ! std::binary_search(_elites.begin(), _elites.end(), *i)*/)
			{
				_elites.push_back(*i);
				_elites.sort(cmp);
			}
#endif
			_elites.push_back(*i);
		}
		_elites.sort(cmp);

		// Too many elites, cull the herd
	} else if (_elites.size() > _num_elites) {
		_elites.sort(cmp);

		// Remove dupes
		if (_elites.size() > 1) {
			typename Elites::iterator i = _elites.begin();
			while (i != _elites.end()) {
				typename Elites::iterator next = i;
				++next;

				if (next != _elites.end() && *i == *next) {
					_elites.erase(i);
				}

				i = next;
			}
		}

		while (_elites.size() > _num_elites) {
			_elites.pop_back();
		}
	}

	assert(_elites.size() == _num_elites);
	if (_problem->fitness_less_than(_best.fitness, _elites.front().fitness)) {
		_best = _elites.front();
	}

	_elite_threshold = _elites.back().fitness;
}

template<typename G>
void
GA<G>::iteration()
{
	++_generation;

	_selection->prepare(_population);

	typename Problem<G>::Population new_population;
	new_population.reserve(_population.size());

	// Elitism (preserve the most fit solutions from last generation)
	new_population.insert(new_population.begin(), _elites.begin(), _elites.end());

	// Crossover parents until new population is as large as the old
	while (new_population.size() < _population.size()) {
		typedef typename Selection<G>::GenePair Parents;
		Parents parents = _selection->select_parents(_rng, _population);

		std::pair<G, G> children = std::make_pair(
			*parents.first, *parents.second);
		if (_crossover && _rng.gamble(_crossover_probability)) {
			// Crossover parents to yield two new children
			children = _crossover->crossover(
				_rng, *parents.first, *parents.second);
			children.first.fitness  = _problem->evaluate(children.first);
			children.second.fitness = _problem->evaluate(children.second);
		}

		// Add both children, or the fittest if there's only room for 1
		if (new_population.size() < _population.size() - 1) {
			new_population.push_back(children.first);
			new_population.push_back(children.second);
		} else if (_problem->fitness_less_than(children.first.fitness,
		                                       children.second.fitness)) {
			new_population.push_back(children.second);
		} else {
			new_population.push_back(children.first);
		}
	}

	// Mutate new population
	for (typename Problem<G>::Population::iterator i = new_population.begin();
	     i != new_population.end(); ++i) {
		if (_rng.gamble(_mutation_probability)) {
			_mutation->mutate(_rng, (G &) * i);
			i->fitness = _problem->evaluate(*i);
			if (_problem->fitness_less_than(_elite_threshold, i->fitness)) {
				_elites.push_back(*i);
			}
		}
	}

	find_elites(new_population);

#ifndef NDEBUG
	for (typename Problem<G>::Population::iterator i = new_population.begin();
	     i != new_population.end(); ++i) {
		assert(_problem->assert_gene(*i));
	}
#endif

	assert(new_population.size() == _population.size());
	_population = new_population;
}

} // namespace eugene

#endif  // EUGENE_GA_HPP
