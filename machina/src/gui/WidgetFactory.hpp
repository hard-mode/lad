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

#include <fstream>
#include <iostream>
#include <string>

#include <gtkmm.h>

#include "machina_config.h"

namespace machina {
namespace gui {

class WidgetFactory
{
public:
	static Glib::RefPtr<Gtk::Builder> create() {
		Glib::RefPtr<Gtk::Builder> xml;

		// Check for the .ui file in current directory
		std::string   ui_filename = "./machina.ui";
		std::ifstream fs(ui_filename.c_str());
		if (fs.fail()) {
			// didn't find it, check MACHINA_DATA_DIR
			fs.clear();
			ui_filename  = MACHINA_DATA_DIR;
			ui_filename += "/machina.ui";

			fs.open(ui_filename.c_str());
			if (fs.fail()) {
				std::cerr << "No machina.ui in current directory or "
				          << MACHINA_DATA_DIR << "." << std::endl;
				exit(EXIT_FAILURE);
			}
			fs.close();
		}

		try {
			xml = Gtk::Builder::create_from_file(ui_filename);
		} catch (const Gtk::BuilderError& ex) {
			std::cerr << ex.what() << std::endl;
			throw ex;
		}

		return xml;
	}

};

}  // namespace machina
}  // namespace gui
