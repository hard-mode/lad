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

#ifndef EUGENE_RANDOM_HPP
#define EUGENE_RANDOM_HPP

#include <random>
#include <limits>

#include <stddef.h>

namespace eugene {

class Random
{
public:
	explicit Random(unsigned value)
		: _engine(value)
		, _norm_dist(0.0f, 1.0f)
		, _uniform_dist(0, std::numeric_limits<unsigned>::max())
	{}

	void seed(unsigned value) { _engine.seed(value); }

	/** Return a natural number between 0 and max inclusive. */
	unsigned natural(unsigned max=std::numeric_limits<unsigned>::max()) {
		return _uniform_dist(_engine) % max;
	}

	/** Return an integer between min and max inclusive. */
	int integer(int min, int max) {
		return natural() % (max - min + 1) + min;
	}

	/** Return a float between 0.0f and 1.0f inclusive. */
	float normal() { return _norm_dist(_engine); }

	/** Return true with probability p. */
	bool gamble(float p) { return normal() < p; }

	/** Return true with probability 0.5 ("flip a coin"). */
	bool flip() { return gamble(0.5f); }

private:
	std::mt19937                            _engine;
	std::normal_distribution<float>         _norm_dist;
	std::uniform_int_distribution<unsigned> _uniform_dist;
};

} // namespace eugene

#endif // EUGENE_RANDOM_HPP
