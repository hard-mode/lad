/*
  This file is part of Machina.
  Copyright 2014 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_MODEL_HPP
#define MACHINA_MODEL_HPP

#include <stdint.h>

#include <map>

#include "machina/Atom.hpp"
#include "machina/types.hpp"

namespace machina {

class Model
{
public:
	virtual ~Model() {}

	virtual void new_object(uint64_t id, const Properties& properties) = 0;

	virtual void erase_object(uint64_t id) = 0;

	virtual void        set(uint64_t id, URIInt key, const Atom& value) = 0;
	virtual const Atom& get(uint64_t id, URIInt key) const = 0;
};

}  // namespace machina

#endif  // MACHINA_MODEL_HPP
