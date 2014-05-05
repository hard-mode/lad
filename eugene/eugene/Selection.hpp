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

#ifndef EUGENE_SELECTION_HPP
#define EUGENE_SELECTION_HPP

#include <atomic>
#include <utility>

#include "eugene/Random.hpp"

namespace eugene {

template<typename G>
class Problem;

template<typename G>
class Selection
{
public:
	typedef typename Problem<G>::Population Population;
	typedef std::pair<typename Problem<G>::Population::iterator,
		typename Problem<G>::Population::iterator>
	GenePair;

	explicit Selection(Problem<G>& problem)
		: _problem(problem)
		, _evaluations(0)
	{}

	virtual ~Selection() {}

	virtual GenePair select_parents(
		Random & rng, typename Problem<G>::Population & pop) const = 0;

	virtual void prepare(typename Problem<G>::Population& pop) const {}

	int evaluations() { return _evaluations; }

protected:
	Problem<G>&                   _problem;
	mutable std::atomic<unsigned> _evaluations;
};

} // namespace eugene

#endif // EUGENE_SELECTION_HPP
