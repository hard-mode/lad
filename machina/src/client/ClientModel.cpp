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

#include "ClientModel.hpp"

namespace machina {
namespace client {

SPtr<ClientObject>
ClientModel::find(uint64_t id)
{
	SPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::iterator  i = _objects.find(key);
	if (i != _objects.end()) {
		return *i;
	} else {
		return SPtr<ClientObject>();
	}
}

SPtr<const ClientObject>
ClientModel::find(uint64_t id) const
{
	SPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::const_iterator  i = _objects.find(key);
	if (i != _objects.end()) {
		return *i;
	} else {
		return SPtr<ClientObject>();
	}
}

void
ClientModel::new_object(uint64_t id, const Properties& properties)
{
	SPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::iterator i = _objects.find(key);
	if (i == _objects.end()) {
		SPtr<ClientObject> object(new ClientObject(id, properties));
		_objects.insert(object);
		signal_new_object.emit(object);
	} else {
		for (const auto& p : properties) {
			(*i)->set(p.first, p.second);
		}
	}
}

void
ClientModel::erase_object(uint64_t id)
{
	SPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::iterator  i = _objects.find(key);
	if (i == _objects.end()) {
		return;
	}

	signal_erase_object.emit(*i);
	(*i)->set_view(NULL);
	_objects.erase(i);
}

void
ClientModel::set(uint64_t id, URIInt key, const Atom& value)
{
	SPtr<ClientObject> object = find(id);
	if (object) {
		object->set(key, value);
	}
}

const Atom&
ClientModel::get(uint64_t id, URIInt key) const
{
	static const Atom null_atom;
	SPtr<const ClientObject> object = find(id);
	return object ? object->get(key) : null_atom;
}

}
}
