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

#ifndef MACHINA_ACTIONFACTORY_HPP
#define MACHINA_ACTIONFACTORY_HPP

#include "machina/types.hpp"

namespace machina {

struct Action;

namespace ActionFactory {
SPtr<Action> copy(SPtr<Action> copy);
SPtr<Action> note_on(uint8_t note, uint8_t velocity=64);
SPtr<Action> note_off(uint8_t note, uint8_t velocity=64);
}

} // namespace machina

#endif // MACHINA_ACTIONFACTORY_HPP
