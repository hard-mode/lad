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

#ifndef EUGENE_TWOPOINTCROSSOVER_HPP
#define EUGENE_TWOPOINTCROSSOVER_HPP

#include <algorithm>
#include <cassert>

#include "eugene/Crossover.hpp"

namespace eugene {

template<typename G>
struct TwoPointCrossover : public Crossover<G> {
	std::pair<G, G> crossover(Random& rng, const G& mom, const G& data) {
		assert(mom.size() == data.size());

		const size_t size = mom.size();

		G child_a(size);
		G child_b(size);

		const size_t rand_1       = rng.natural(size);
		const size_t rand_2       = rng.natural(size);
		const size_t chop_1_index = std::min(rand_1, rand_2);
		const size_t chop_2_index = std::max(rand_1, rand_2);

		for (size_t i = 0; i < chop_1_index; ++i) {
			child_a[i] = mom[i];
			child_b[i] = data[i];
		}

		for (size_t i = chop_1_index; i < chop_2_index; ++i) {
			child_a[i] = data[i];
			child_b[i] = mom[i];
		}

		for (size_t i = chop_2_index; i < size; ++i) {
			child_a[i] = mom[i];
			child_b[i] = data[i];
		}

		assert(child_a.size() == mom.size());
		assert(child_a.size() == child_b.size());

		return make_pair(child_a, child_b);
	}
};

} // namespace eugene

#endif  // EUGENE_TWOPOINTCROSSOVER_HPP
