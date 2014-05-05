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

#ifndef EUGENE_PARTIALLYMAPPEDCROSSOVER_HPP
#define EUGENE_PARTIALLYMAPPEDCROSSOVER_HPP

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <utility>

#include "eugene/Crossover.hpp"

namespace eugene {

template<typename G>
class PartiallyMappedCrossover : public Crossover<G>
{
public:
	std::pair<G, G> crossover(Random& rng, const G& mom, const G& dad) {
		assert(mom.size() == dad.size());
		const size_t gene_size = mom.size();

		std::numeric_limits<typename G::value_type> value_limits;

		G child_a(gene_size, value_limits.max());
		G child_b(gene_size, value_limits.max());

		const size_t rand_1 = rng.natural(gene_size);
		size_t       rand_2 = rng.natural(gene_size);
		while (rand_2 == rand_1) {
			rand_2 = rng.natural(gene_size);
		}

		const size_t cut_a = std::min(rand_1, rand_2);
		const size_t cut_b = std::max(rand_1, rand_2);

		// Copy a chunk from parent1->child_a && parent2->child_b
		for (size_t i = cut_a; i < cut_b; ++i) {
			child_a[i] = mom[i];
		}
		child_a[cut_b] = mom[cut_b];

		for (size_t i = cut_a; i < cut_b; ++i) {
			child_b[i] = dad[i];
		}
		child_b[cut_b] = dad[cut_b];

		fill_in_order(dad, child_a, cut_a, cut_b);
		fill_in_order(mom, child_b, cut_a, cut_b);

		return make_pair(*(G*)&child_a, *(G*)&child_b);
	}

private:
	inline static void fill_in_order(const G&     parent,
	                                 G&           child,
	                                 const size_t left,
	                                 const size_t right)
	{
		assert(parent.size() == child.size());

		std::numeric_limits<typename G::value_type> value_limits;

		// Map cities from parent which weren't copied from other parent
		for (size_t i = (right + 1) % child.size();
		     i != left;
		     i = (i + 1) % child.size()) {
			if (!Crossover<G>::contains(child, left, right, parent[i])) {
				child[i] = parent[i];
			} else {
				child[i] = value_limits.max();
			}
		}

		size_t parent_read = 0;

		// Fill the remaining gaps with cities from parent in order
		for (size_t i = 0; i < child.size(); ++i) {
			if (child[i] >= child.size()) {
				while (Crossover<G>::contains(child, 0, child.size() - 1,
				                              parent[parent_read])) {
					++parent_read;
				}

				if (parent_read >= parent.size()) {
					break;
				}

				assert(parent[parent_read] < parent.size());

				assert(parent_read < parent.size());

				child[i] = parent[parent_read];

				++parent_read;
			}
		}
	}

};

} // namespace eugene

#endif // EUGENE_PARTIALLYMAPPEDCROSSOVER_HPP
