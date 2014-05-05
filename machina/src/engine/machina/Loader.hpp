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

#ifndef MACHINA_LOADER_HPP
#define MACHINA_LOADER_HPP

#include <glibmm/ustring.h>

#include "machina/Atom.hpp"
#include "machina/types.hpp"
#include "raul/TimeStamp.hpp"
#include "sord/sordmm.hpp"

using Sord::Namespaces;

namespace machina {

class Machine;

class Loader
{
public:
	Loader(Forge& forge, Sord::World& rdf_world);

	SPtr<Machine> load(const Glib::ustring& filename);

	SPtr<Machine> load_midi(const Glib::ustring& filename,
	                        double               q,
	                        Raul::TimeDuration   dur);

private:
	Forge&       _forge;
	Sord::World& _rdf_world;
};

} // namespace machina

#endif // MACHINA_LOADER_HPP
