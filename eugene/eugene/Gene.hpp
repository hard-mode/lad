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

#ifndef EUGENE_GENE_HPP
#define EUGENE_GENE_HPP

#include <math.h>

#include <vector>
#include <ostream>

namespace eugene {

template<typename A>
struct Gene : public std::vector<A>{
	Gene(size_t gene_size, const A& initial = A())
		: std::vector<A>(gene_size, initial)
		, fitness(NAN)
	{}

	inline bool operator==(const Gene<A>& other) const {
		for (size_t i = 0; i < this->size(); ++i) {
			if ((*this)[i] != other[i]) {
				return false;
			}
		}

		return true;
	}

	inline bool operator!=(const Gene<A>& other) const {
		return !operator==(other);
	}

	float fitness;
};

template<typename A>
inline std::ostream&
operator<<(std::ostream& os, const Gene<A>& g) {
	os << g[0];
	for (size_t i = 0; i < g.size(); ++i) {
		os << ", " << g[i];
	}
	return os << std::endl;
}

} // namespace eugene

#endif // EUGENE_GENE_HPP
