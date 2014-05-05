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

#ifndef EUGENE_RANKSELECTION_HPP
#define EUGENE_RANKSELECTION_HPP

#include <math.h>

#include "eugene/Selection.hpp"
#include "eugene/Problem.hpp"

namespace eugene {

template<typename G>
struct RankSelection : public Selection<G>
{
	RankSelection(Problem<G>& problem, float p)
		: Selection<G>(problem)
		, _p(p)
	{}

	void prepare(typename Problem<G>::Population& pop) const {
		typename Problem<G>::FitnessGreaterThan cmp(Selection<G>::_problem);
		sort(pop.begin(), pop.end(), cmp);
	}

	typename Selection<G>::GenePair
	select_parents(Random& rng, typename Problem<G>::Population& pop) const
	{
		/* FIXME: slooow */

		typename Problem<G>::Population::const_iterator i;

		float total = 0;
		for (size_t i = 0; i < pop.size(); ++i) {
			total += powf(_p, (float)i);
		}

		typename Selection<G>::GenePair result = make_pair(pop.end(), pop.end());

		float accum = 0;

		const float spin_1 = rng.normal() * total;
		const float spin_2 = rng.normal() * total;

		for (size_t i = 0; i < pop.size(); ++i) {
			accum += powf(_p, (float)i);

			if ((result.first == pop.end()) && (accum >= spin_1)) {
				result.first = pop.begin() + i;
				if (result.second != pop.end()) {
					break;
				}
			}

			if (( result.second == pop.end()) && ( accum >= spin_2) ) {
				result.second = pop.begin() + i;
				if (result.first != pop.end()) {
					break;
				}
			}
		}

		return result;
	}

private:
	float _p;
};

} // namespace eugene

#endif // EUGENE_RANKSELECTION_HPP
