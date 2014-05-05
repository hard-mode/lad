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

#ifndef EUGENE_CROSSOVER_HPP
#define EUGENE_CROSSOVER_HPP

#include <iostream>
#include <utility>
#include <cassert>

#include "eugene/Random.hpp"

namespace eugene {

template<typename G>
struct Crossover {
	virtual ~Crossover() {}

	virtual std::pair<G, G> crossover(Random&  rng,
	                                  const G& mom,
	                                  const G& dad) = 0;

	template<typename A>
	inline static bool contains(const G&     gene,
	                            const size_t left,
	                            size_t       right,
	                            A            value)
	{
		assert(left != right);
		assert(right < gene.size());
		assert(left < gene.size());

		if (gene[right] == value) {
			return true;
		}

		for (size_t i = left; i != right; i = (i + 1) % gene.size()) {
			if (gene[i] == value) {
				return true;
			}
		}

		return false;
	}

};

} // namespace eugene

#endif // EUGENE_CROSSOVER_HPP
