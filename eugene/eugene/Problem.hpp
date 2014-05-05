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

#ifndef EUGENE_PROBLEM_HPP
#define EUGENE_PROBLEM_HPP

#include <cassert>
#include <cstddef>

#include <vector>

#include "eugene/Gene.hpp"
#include "eugene/Random.hpp"

namespace eugene {

template<typename G>
class Problem
{
public:
	explicit Problem(size_t gene_size = 0)
		: _gene_size(gene_size) {}
	virtual ~Problem() {}

	typedef std::vector<G> Population;

	virtual float evaluate(const G& g) const = 0;

	virtual bool fitness_less_than(float a, float b) const = 0;

	inline float total_fitness(const Population& pop) const {
		float result = 0.0f;

		for (size_t i = 0; i < pop.size(); ++i) {
			result += pop[i].fitness;
		}

		return result;
	}

#ifndef NDEBUG
	virtual bool assert_gene(const G& g) const { return true; }
#endif

	size_t gene_size() { return _gene_size; }

	virtual void initial_population(Random&     rng,
	                                Population& pop,
	                                size_t      gene_size,
	                                size_t      pop_size) const = 0;

	virtual bool  optimum_known() { return false; }
	virtual float optimum() const { return 0; }

	struct FitnessGreaterThan {
		explicit FitnessGreaterThan(Problem<G>& problem) : _problem(problem) {}
		inline bool operator()(const G& a, const G& b) const {
			return _problem.fitness_less_than(b.fitness, a.fitness);
		}

		Problem<G>& _problem;
	};

protected:
	size_t _gene_size;
};

} // namespace eugene

#endif // EUGENE_PROBLEM_HPP
