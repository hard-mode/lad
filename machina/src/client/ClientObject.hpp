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

#ifndef MACHINA_CLIENTOBJECT_HPP
#define MACHINA_CLIENTOBJECT_HPP

#include <map>

#include <sigc++/sigc++.h>

#include "machina/Atom.hpp"
#include "machina/types.hpp"

namespace machina {
namespace client {

class ClientObject
{
public:
	explicit ClientObject(uint64_t id);
	ClientObject(const ClientObject& copy, uint64_t id);

	inline uint64_t id() const { return _id; }

	void        set(URIInt key, const Atom& value);
	const Atom& get(URIInt key) const;

	sigc::signal<void, URIInt, Atom> signal_property;

	class View
	{
	public:
		virtual ~View() {}
	};

	typedef std::map<URIInt, Atom> Properties;
	const Properties& properties() { return _properties; }

	View* view() const         { return _view; }
	void  set_view(View* view) { _view = view; }

private:
	uint64_t _id;
	View*    _view;

	Properties _properties;
};

/** Stub client object to use as a search key. */
struct ClientObjectKey : public ClientObject
{
	explicit ClientObjectKey(uint64_t id) : ClientObject(id) {}
};

}
}

#endif // MACHINA_CLIENTOBJECT_HPP
