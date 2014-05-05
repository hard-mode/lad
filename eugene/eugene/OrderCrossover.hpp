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

#ifndef EUGENE_ORDERCROSSOVER_HPP
#define EUGENE_ORDERCROSSOVER_HPP

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "eugene/Crossover.hpp"

namespace eugene {

template<typename G>
class OrderCrossover : public Crossover<G>
{
public:
	std::pair<G, G> crossover(Random& rng, const G& mom, const G& data) {
		assert(mom.size() == data.size());

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
		for (size_t i = cut_a; i <= cut_b; ++i) {
			child_a[i] = mom[i];
		}
		for (size_t i = cut_a; i <= cut_b; ++i) {
			child_b[i] = data[i];
		}

		const size_t left  = cut_a;
		const size_t right = (cut_b + 1) % gene_size;

		// Fill in the rest with cities in order from the other parent
		fill_in_order(data, child_a, left, right);
		fill_in_order(mom, child_b, left, right);

		return make_pair(*(G*)&child_a, *(G*)&child_b);
	}

private:
	inline static void fill_in_order(const G&     parent,
	                                 G&           child,
	                                 const size_t left,
	                                 size_t       right)
	{
		assert(parent.size() == child.size());
		size_t parent_read = right;

		while (right != left) {
			if (!Crossover<G>::contains(
				    child, left, right,parent[parent_read])) {
				child[right] = parent[parent_read];
				right        = (right + 1) % parent.size();
			}

			parent_read = (parent_read + 1) % parent.size();
		}
	}

};

} // namespace eugene

#endif // EUGENE_ORDERCROSSOVER_HPP
