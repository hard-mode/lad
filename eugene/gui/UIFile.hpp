/* This file is part of Eugene
 * Copyright 2007-2012 David Robillard <http://drobilla.net>
 *
 * Eugene is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Eugene is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Eugene.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EUGENE_UIFILE_HPP
#define EUGENE_UIFILE_HPP

#include <fstream>
#include <iostream>
#include <string>

#include <gtkmm/builder.h>

#include "eugene_config.h"

class UIFile {
public:
	inline static bool is_readable(const std::string& filename) {
		std::ifstream fs(filename.c_str());
		const bool fail = fs.fail();
		fs.close();
		return !fail;
	}

	static Glib::RefPtr<Gtk::Builder> open(const std::string& base_name) {
		std::string ui_filename = std::string(EUGENE_DATA_DIR)
			+ "/" + base_name + ".ui";
		if (is_readable(ui_filename)) {
			std::cout << "Loading UI file " << ui_filename << std::endl;
			return Gtk::Builder::create_from_file(ui_filename);
		}

		std::cerr << "Failed to open UI " << ui_filename << std::endl;
		return Glib::RefPtr<Gtk::Builder>();
	}
};

#endif // EUGENE_UIFILE_HPP
