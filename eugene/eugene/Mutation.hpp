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

#ifndef EUGENE_MUTATION_HPP
#define EUGENE_MUTATION_HPP

#include <algorithm>
#include <cassert>

#include "eugene/Random.hpp"

namespace eugene {

template<typename G>
struct Mutation {
	virtual ~Mutation() {}
	virtual void mutate(Random& rng, G& g) = 0;
};

template<typename G>
class RandomMutation : public Mutation<G>
{
public:
	RandomMutation(typename G::value_type min, typename G::value_type max)
		: _min(min)
		, _max(max)
	{}

	void mutate(Random& rng, G& g) {
		const size_t index = rng.natural(g.size());
		g[index] = rng.integer(_min, _max);
	}

private:
	typename G::value_type _min;
	typename G::value_type _max;
};

template<typename G>
class FlipMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		const size_t index = rng.natural(g.size());
		g[index] *= -1;
	}

};

template<typename G>
class FlipRangeMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		const size_t rand_1 = rng.natural(g.size());
		const size_t rand_2 = rng.natural(g.size());
		const size_t left   = std::min(rand_1, rand_2);
		const size_t right  = std::max(rand_1, rand_2);

		for (size_t i = left; i <= right; ++i) {
			assert(g[i] == 1 || g[i] == -1);
			g[i] *= -1;
		}
	}

};

template<typename G>
class FlipRandomMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		for (size_t i = 0; i < g.size(); ++i) {
			assert(g[i] == 1 || g[i] == -1);
			if (rng.gamble(1 / (g.size() / 2.0f)) == 0) {
				g[i] *= -1;
			}
		}
	}

};

template<typename G>
class SwapRangeMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		const size_t index_a = rng.natural(g.size());
		const size_t index_b = rng.natural(g.size());
		typename G::value_type temp = g[index_a];
		g[index_a]                  = g[index_b];
		g[index_b]                  = temp;
	}

};

template<typename G>
class SwapAlleleMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		const size_t index_a = rng.natural(g.size());
		const size_t index_b = rng.natural(g.size());
		typename G::value_type temp = g[index_a];
		g[index_a]                  = g[index_b];
		g[index_b]                  = temp;
	}

};

template<typename G>
class ReverseMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		const size_t index_a = rng.natural(g.size());
		const size_t index_b = rng.natural(g.size());
		const size_t left    = std::min(index_a, index_b);
		const size_t right   = std::max(index_a, index_b);

		for (size_t i = 0; i < right - left; ++i) {
			typename G::value_type temp = g[left + i];
			g[left + i]                 = g[right - i];
			g[right - i]                = temp;
		}
	}

};

template<typename G>
class PermuteMutation : public Mutation<G>
{
public:
	void mutate(Random& rng, G& g) {
		std::random_shuffle(g.begin(), g.end());
	}

};

template<typename G>
class NullMutation : public Mutation<G>
{
public:
	inline static void mutate(Random& rng, G& g) {}
};

} // namespace eugene

#endif // EUGENE_MUTATION_HPP
