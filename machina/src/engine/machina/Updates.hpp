/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_UPDATES_HPP
#define MACHINA_UPDATES_HPP

#include <stdint.h>

#include "machina/Atom.hpp"
#include "machina/types.hpp"

namespace machina {

enum UpdateType {
	UPDATE_SET = 1
};

void
write_set(SPtr<Raul::RingBuffer> buf,
          uint64_t               subject,
          URIInt                 key,
          const Atom&            value);

uint32_t
read_set(SPtr<Raul::RingBuffer> buf,
         uint64_t*              subject,
         URIInt*                key,
         Atom*                  value);

} // namespace machina

#endif // MACHINA_UPDATES_HPP
