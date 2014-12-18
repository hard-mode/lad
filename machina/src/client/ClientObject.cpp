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

#include <iostream>

#include "ClientObject.hpp"

namespace machina {
namespace client {

ClientObject::ClientObject(uint64_t id, const Properties& properties)
	: _id(id)
	, _view(NULL)
	, _properties(properties)
{}

ClientObject::ClientObject(const ClientObject& copy, uint64_t id)
	: _id(id)
	, _view(NULL)
	, _properties(copy._properties)
{}

void
ClientObject::set(URIInt key, const Atom& value)
{
	_properties[key] = value;
	signal_property.emit(key, value);
}

const Atom&
ClientObject::get(URIInt key) const
{
	static const Atom          null_atom;
	Properties::const_iterator i = _properties.find(key);
	if (i != _properties.end()) {
		return i->second;
	} else {
		return null_atom;
	}
}

}
}
