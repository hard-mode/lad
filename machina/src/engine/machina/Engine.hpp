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

#ifndef MACHINA_ENGINE_HPP
#define MACHINA_ENGINE_HPP

#include <string>

#include <glibmm/ustring.h>

#include "machina/Atom.hpp"
#include "machina/Driver.hpp"
#include "machina/Loader.hpp"
#include "machina/types.hpp"

namespace machina {

class Machine;

class Engine
{
public:
	Engine(Forge&       forge,
	       SPtr<Driver> driver,
	       Sord::World& rdf_world);

	Sord::World& rdf_world() { return _rdf_world; }

	static SPtr<Driver> new_driver(Forge&             forge,
	                               const std::string& name,
	                               SPtr<Machine>      machine);

	SPtr<Driver>  driver()  { return _driver; }
	SPtr<Machine> machine() { return _driver->machine(); }
	Forge&        forge()   { return _forge; }

	SPtr<Machine> load_machine(const Glib::ustring& uri);
	SPtr<Machine> load_machine_midi(const Glib::ustring& uri,
	                                double               q,
	                                Raul::TimeDuration   dur);

	void export_midi(const Glib::ustring& filename,
	                 Raul::TimeDuration   dur);

	void set_bpm(double bpm);
	void set_quantization(double beat_fraction);

private:
	SPtr<Driver> _driver;
	Sord::World& _rdf_world;
	Loader       _loader;
	Forge        _forge;
};

} // namespace machina

#endif // MACHINA_ENGINE_HPP
