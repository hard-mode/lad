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

#include <glibmm/ustring.h>

#include "machina_config.h"
#include "machina/Engine.hpp"
#include "machina/Loader.hpp"
#include "machina/Machine.hpp"
#include "SMFDriver.hpp"
#ifdef HAVE_JACK
#include "JackDriver.hpp"
#endif

namespace machina {

Engine::Engine(Forge&       forge,
               SPtr<Driver> driver,
               Sord::World& rdf_world)
	: _driver(driver)
	, _rdf_world(rdf_world)
	, _loader(_forge, _rdf_world)
	, _forge(forge)
{}

SPtr<Driver>
Engine::new_driver(Forge&             forge,
                   const std::string& name,
                   SPtr<Machine>      machine)
{
#ifdef HAVE_JACK
	if (name == "jack") {
		JackDriver* driver = new JackDriver(forge, machine);
		driver->attach("machina");
		return SPtr<Driver>(driver);
	}
#endif
	if (name == "smf") {
		return SPtr<Driver>(new SMFDriver(forge, machine->time().unit()));
	}

	std::cerr << "Error: Unknown driver type `" << name << "'" << std::endl;
	return SPtr<Driver>();
}

/** Load the machine at `uri`, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SPtr<Machine>
Engine::load_machine(const Glib::ustring& uri)
{
	SPtr<Machine> machine     = _loader.load(uri);
	SPtr<Machine> old_machine = _driver->machine();  // Keep reference
	if (machine) {
		_driver->set_machine(machine);  // Switch driver to new machine and wait
	}

	// Drop (possibly last) reference to old_machine in this thread
	return machine;
}

/** Build a machine from the MIDI at `uri`, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SPtr<Machine>
Engine::load_machine_midi(const Glib::ustring& uri,
                          double               q,
                          Raul::TimeDuration   dur)
{
	SPtr<Machine> machine     = _loader.load_midi(uri, q, dur);
	SPtr<Machine> old_machine = _driver->machine();  // Keep reference
	if (machine) {
		_driver->set_machine(machine);  // Switch driver to new machine and wait
	}

	// Drop (possibly last) reference to old_machine in this thread
	return machine;
}

void
Engine::export_midi(const Glib::ustring& filename, Raul::TimeDuration dur)
{
	SPtr<Machine>            machine = _driver->machine();
	SPtr<machina::SMFDriver> file_driver(
		new machina::SMFDriver(_forge, dur.unit()));

	const bool activated = _driver->is_activated();
	if (activated) {
		_driver->deactivate(); // FIXME: disable instead
	}
	file_driver->writer()->start(filename, TimeStamp(dur.unit(), 0.0));
	file_driver->run(machine, dur);
	machine->reset(NULL, machine->time());
	file_driver->writer()->finish();

	if (activated) {
		_driver->activate();
	}
}

void
Engine::set_bpm(double bpm)
{
	_driver->set_bpm(bpm);
}

void
Engine::set_quantization(double q)
{
	_driver->set_quantization(q);
}

} // namespace machina
