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

#ifndef EUGENE_POSITIONBASEDCROSSOVER_HPP
#define EUGENE_POSITIONBASEDCROSSOVER_HPP

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <utility>

#include "eugene/Crossover.hpp"

namespace eugene {

template<typename G>
struct PositionBasedCrossover : public Crossover<G> {
	// Fill in gaps (elements >= size) with elements from parent in order
	inline static void fill_gaps(const G& parent, G& child)
	{
		assert(parent.size() == child.size());

		size_t child_write = 0;

		for (size_t i = 0; i < parent.size(); ++i) {
			while (child[child_write] < child.size()) {
				if (child_write < child.size()) {
					++child_write;
				} else {
					return;
				}
			}

			if (!Crossover<G>::contains(child, 0, child.size() - 1,
			                            parent[i])) {
				child[child_write++] = parent[i];
			}
		}
	}

	std::pair<G, G> crossover(Random& rng, const G& mom, const G& dad) {
		assert(mom.size() == dad.size());
		const size_t gene_size = mom.size();

		G child_a(gene_size);
		G child_b(gene_size);

		for (size_t i = 0; i < gene_size; ++i) {
			if (rng.flip()) {
				child_a[i] = mom[i];
				child_b[i] = dad[i];
			} else {
				child_a[i] = gene_size;
				child_b[i] = gene_size;
			}
		}

		fill_gaps(mom, child_b);
		fill_gaps(dad, child_a);

		return make_pair(*(G*)&child_a, *(G*)&child_b);
	}
};

} // namespace eugene

#endif // EUGENE_POSITIONBASEDCROSSOVER_HPP
