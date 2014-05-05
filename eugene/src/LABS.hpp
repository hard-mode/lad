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

#ifndef EUGENE_LABS_HPP
#define EUGENE_LABS_HPP

#include <utility>
#include <string>
#include <set>
#include <cmath>

#include "eugene/Problem.hpp"
#include "eugene/Gene.hpp"

namespace eugene {

class LABS : public Problem< Gene<int32_t> > {
public:
	explicit LABS(size_t gene_size) : Problem< Gene<int32_t> >(gene_size) {}

	typedef int32_t      Allele;
	typedef Gene<Allele> GeneType;

	float evaluate(const GeneType& g) const;

	bool fitness_less_than(float a, float b) const { return a > b; }

	void initial_population(Random&     rng,
	                        Population& pop,
	                        size_t      gene_size,
	                        size_t      pop_size) const;

#ifndef NDEBUG
	bool assert_gene(const GeneType& g) const {
		for (size_t i = 0; i < g.size(); ++i) {
			if (g[i] != 1 && g[i] != -1)
				return false;
		}

		return true;
	}
#endif

	bool optimum_known() { return (_gene_size >= 3 && _gene_size <= 140); }
	float optimum() const;

private:

	inline int32_t c_g(size_t n, size_t g, const GeneType& s) const {
		int32_t ret = 0;

		for (size_t i=0; i < (n - g); ++i) {
			assert(i < s.size());
			assert((i + g) < s.size());
			ret += s[i] * s[i + g];
		}

		return ret;
	}
};

} // namespace eugene

#endif  // EUGENE_LABS_HPP
