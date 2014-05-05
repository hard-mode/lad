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

#ifndef EUGENE_HYBRIDMUTATION_HPP
#define EUGENE_HYBRIDMUTATION_HPP

#include <memory>
#include <vector>

#include "eugene/Mutation.hpp"

namespace eugene {

/** A probabilistic composite of several mutation functions
 */
template<typename G>
class HybridMutation : public Mutation<G>
{
public:
	typedef std::vector< std::pair< float,
			std::shared_ptr< Mutation<G> > > > Mutations;

	void append_mutation(float probability, std::shared_ptr< Mutation<G> > c) {
		_mutations.push_back(make_pair(probability, c));
	}

	void set_probability(size_t mutation_index, float probability) {
		const float delta  = probability - _mutations[mutation_index].first;
		const float others = 1.0 - _mutations[mutation_index].first;

		if (others == 0.0f) {
			return; // bad user, no cookie
		}
		_mutations[mutation_index].first = probability;

		if (probability == 1.0) {
			for (size_t i = 0; i < _mutations.size(); ++i) {
				if (i != mutation_index) {
					_mutations[i].first = 0.0f;
				}
			}
		} else {
			for (size_t i = 0; i < _mutations.size(); ++i) {
				if (i != mutation_index) {
					_mutations[i].first -= delta
					    * (_mutations[i].first / others);
				}
			}
		}
	}

	void mutate(Random& rng, G& gene) {
		const float cut = rng.normal();

		float accum = 0;
		for (typename Mutations::iterator i = _mutations.begin();
		     i != _mutations.end(); ++i) {
			accum += i->first;
			if (cut <= accum) {
				i->second->mutate(rng, gene);
				return;
			}
		}

		_mutations.back().second->mutate(rng, gene);
	}

	const Mutations& mutations() const { return _mutations; }

private:
	Mutations _mutations;
};

} // namespace eugene

#endif  // EUGENE_HYBRIDMUTATION_HPP
