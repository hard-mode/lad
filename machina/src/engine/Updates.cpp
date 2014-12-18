/*
  This file is part of Machina.
  Copyright 2007-2014 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "machina/Atom.hpp"
#include "machina/Updates.hpp"
#include "machina/types.hpp"

namespace machina {

void
write_set(SPtr<Raul::RingBuffer> buf,
          uint64_t               subject,
          URIInt                 key,
          const Atom&            value)
{
	const uint32_t update_type = UPDATE_SET;
	buf->write(sizeof(update_type), &update_type);
	buf->write(sizeof(subject), &subject);
	buf->write(sizeof(key), &key);

	const LV2_Atom atom = { value.size(), value.type() };
	buf->write(sizeof(LV2_Atom), &atom);
	buf->write(value.size(), value.get_body());
}

uint32_t
read_set(SPtr<Raul::RingBuffer> buf,
         uint64_t*              subject,
         URIInt*                key,
         Atom*                  value)
{
	uint32_t update_type = 0;
	buf->read(sizeof(update_type), &update_type);
	if (update_type != UPDATE_SET) {
		return 0;
	}

	buf->read(sizeof(*subject), subject);
	buf->read(sizeof(*key), key);

	LV2_Atom atom = { 0, 0 };
	buf->read(sizeof(LV2_Atom), &atom);
	*value = Atom(atom.size, atom.type, NULL);
	buf->read(atom.size, value->get_body());

	return sizeof(update_type) + sizeof(*subject) + sizeof(*key)
	       + sizeof(LV2_Atom) + atom.size;
}

}
