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

#include <iostream>
#include <string>
#include "raul/log.hpp"
#include "raul/SMFReader.hpp"
#include "raul/SMFWriter.hpp"

using namespace std;
using namespace machina;

int
main(int argc, char** argv)
{
#define CHECK(cond) \
	do { if (!(cond)) { \
			error << "Test at " << __FILE__ << ":" << __LINE__ \
			      << " failed: " << __STRING(cond) << endl; \
			return 1; \
		} \
	} \
	while (0)

	static const uint16_t ppqn = 19200;

	const char* filename = NULL;

	if (argc < 2) {
		filename = "./test.mid";
		SMFWriter writer(TimeUnit(TimeUnit::BEATS, ppqn));
		writer.start(string(filename), TimeStamp(writer.unit(), 0, 0));
		writer.finish();
	} else {
		filename = argv[1];
	}

	SMFReader reader;
	bool      opened = reader.open(filename);

	if (!opened) {
		cerr << "Unable to open SMF file " << filename << endl;
		return -1;
	}

	CHECK(reader.type() == 0);
	CHECK(reader.num_tracks() == 1);
	CHECK(reader.ppqn() == ppqn);

	for (unsigned t = 1; t <= reader.num_tracks(); ++t) {
		reader.seek_to_track(t);

		unsigned char buf[4];
		uint32_t      ev_size;
		uint32_t      ev_delta_time;
		while (reader.read_event(4, buf, &ev_size, &ev_delta_time) >= 0) {

			cout << t << ": Event, size = " << ev_size << ", time = "
			     << ev_delta_time;
			cout << ":\t";
			cout.flags(ios::hex);
			for (uint32_t i = 0; i < ev_size; ++i) {
				cout << "0x" << static_cast<int>(buf[i]) << " ";
			}
			cout.flags(ios::dec);
			cout << endl;
		}
	}

	return 0;
}
