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

#ifndef EUGENE_TOURNAMENTSELECTION_HPP
#define EUGENE_TOURNAMENTSELECTION_HPP

#include <cassert>
#include <vector>

#include "eugene/Selection.hpp"

namespace eugene {

template<typename G>
struct TournamentSelection : public Selection<G>
{
	TournamentSelection(Problem<G>& problem, size_t n, float p)
		: Selection<G>(problem)
		, _n(n)
		, _p(p)
	{
		assert(p >= 0.0f && p <= 1.0f);
		assert(_n > 0);
	}

	typename Selection<G>::GenePair
	select_parents(Random& rng, typename Problem<G>::Population& pop) const
	{
		assert(pop.size() >= _n);
		typename Problem<G>::Population::iterator mom = select_parent(rng, pop);
		typename Problem<G>::Population::iterator dad = select_parent(rng, pop);
		while (mom == dad) {
			dad = select_parent(rng, pop);
		}

		return make_pair(mom, dad);
	}

	typename Problem<G>::Population::iterator
	select_parent(Random& rng, typename Problem<G>::Population& pop) const
	{
		std::vector<size_t> players;
		players.reserve(_n);

		while (players.size() < _n) {
			size_t index = rng.natural(pop.size());
			while (find(players.begin(), players.end(), index)
			       != players.end()) {
				index = rng.natural(pop.size());
			}

			players.push_back(index);
		}

		if (rng.normal() < _p) {
			typename Problem<G>::Population::iterator ret = pop.begin()
			    + players[0];
			for (size_t i = 1; i < _n; ++i) {
				++Selection<G>::_evaluations;
				if ((Selection<G>::_problem).fitness_less_than(
				        ret->fitness, (pop.begin() + players[i])->fitness)) {
					ret = pop.begin() + players[i];
				}
			}

			return ret;
		} else {
			return pop.begin() + players[rng.natural(_n)];
		}
	}

private:
	size_t _n;
	float  _p;
};

} // namespace eugene

#endif // EUGENE_TOURNAMENTSELECTION_HPP
