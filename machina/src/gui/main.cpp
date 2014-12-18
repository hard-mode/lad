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

#include <signal.h>

#include <iostream>
#include <string>

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "machina/Engine.hpp"
#include "machina/Loader.hpp"
#include "machina/Machine.hpp"
#include "machina/URIs.hpp"
#include "machina_config.h"
#include "sord/sordmm.hpp"

#include "MachinaGUI.hpp"

using namespace std;
using namespace machina;

int
main(int argc, char** argv)
{
	Glib::thread_init();
	machina::URIs::init();

	Sord::World rdf_world;
	rdf_world.add_prefix("", MACHINA_NS);
	rdf_world.add_prefix("midi", LV2_MIDI_PREFIX);

	Forge                  forge;
	SPtr<machina::Machine> machine;

	Raul::TimeUnit beats(TimeUnit::BEATS, MACHINA_PPQN);

	// Load machine, if given
	if (argc >= 2) {
		const string filename = argv[1];
		const string ext      = filename.substr(filename.length() - 4);

		if (ext == ".ttl") {
			cout << "Loading machine from " << filename << endl;
			machine = Loader(forge, rdf_world).load(filename);

		} else if (ext == ".mid") {
			cout << "Building machine from MIDI file " << filename << endl;

			double q = 0.0;
			if (argc >= 3) {
				q = strtod(argv[2], NULL);
				cout << "Quantization: " << q << endl;
			}

			machine = Loader(forge, rdf_world).load_midi(
				filename, q, Raul::TimeDuration(beats, 0, 0));
		}

		if (!machine) {
			cerr << "Failed to load machine, exiting" << std::endl;
			return 1;
		}
	}


	if (!machine) {
		machine = SPtr<Machine>(new Machine(beats));
	}

	// Create driver
	SPtr<Driver> driver(Engine::new_driver(forge, "jack", machine));
	if (!driver) {
		cerr << "warning: Failed to create Jack driver, using SMF" << endl;
		driver = SPtr<Driver>(Engine::new_driver(forge, "smf", machine));
	}

	SPtr<Engine> engine(new Engine(forge, driver, rdf_world));

	Gtk::Main app(argc, argv);

	driver->activate();
	gui::MachinaGUI gui(engine);

	app.run(*gui.window());

	return 0;
}
