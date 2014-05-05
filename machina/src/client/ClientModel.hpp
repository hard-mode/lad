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

#ifndef MACHINA_CLIENTMODEL_HPP
#define MACHINA_CLIENTMODEL_HPP

#include <set>

#include <sigc++/sigc++.h>

#include "ClientObject.hpp"

namespace Raul {
class Atom;
}

namespace machina {
namespace client {

class ClientModel
{
public:
	void new_object(SPtr<ClientObject> object);
	void erase_object(uint64_t id);
	void property(uint64_t id, URIInt key, const Atom& value);

	SPtr<ClientObject> find(uint64_t id);

	sigc::signal< void, SPtr<ClientObject> > signal_new_object;
	sigc::signal< void, SPtr<ClientObject> > signal_erase_object;

private:
	struct ClientObjectComparator {
		inline bool operator()(SPtr<ClientObject> lhs,
		                       SPtr<ClientObject> rhs) const {
			return lhs->id() < rhs->id();
		}

	};

	typedef std::set<SPtr<ClientObject>, ClientObjectComparator> Objects;
	Objects _objects;
};

}
}

#endif // MACHINA_CLIENTMODEL_HPP
