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

#ifndef EUGENE_TSP_HPP
#define EUGENE_TSP_HPP

#include <stdint.h>
#include <math.h>

#include <utility>
#include <string>
#include <set>

#include "eugene/Problem.hpp"
#include "eugene/Gene.hpp"

namespace eugene {

class TSP : public Problem< Gene<uint32_t> > {
public:
	typedef Gene<uint32_t> GeneType;

	TSP() {}
	explicit TSP(const std::string& filename);

	float evaluate(const GeneType& g) const;
	bool fitness_less_than(float a, float b) const { return a > b; }

	typedef std::pair<uint32_t,uint32_t> City;
	typedef std::vector<City>            Cities;

	void initial_population(Random&     rng,
	                        Population& pop,
	                        size_t      gene_size,
	                        size_t      pop_size) const;

	const Cities& cities() const { return _cities; }

#ifndef NDEBUG
	/* void print_tsp_gene(const typename G::const_iterator l, const typename G::const_iterator r) {
		for (typename G::const_iterator i=l; i != r; ++i)
			std::cout << *i << ",";
	}*/

	bool assert_gene(const GeneType& g) const {
		std::set<GeneType::value_type> values;
		for (GeneType::const_iterator i = g.begin(); i != g.end(); ++i) {
			if (values.find(*i) != values.end())
				return false;
			else if (*i >= g.size())
				return false;
			else
				values.insert(*i);
		}

		return true;
	}
#endif

private:

	inline float distance(const City& a, const City& b) const {
		// FIXME: this will blow up if the distances exceed int32_t range..
		int32_t dx = b.first - a.first;
		int32_t dy = b.second - a.second;
		dx *= dx;
		dy *= dy;
		return sqrt(dx + dy);
	}

	const std::string _filename;
	Cities _cities;
};

} // namespace eugene

#endif  // EUGENE_TSP_HPP
