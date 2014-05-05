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

#include "quantize.hpp"

using namespace std;
using namespace machina;

int
main()
{
	TimeStamp q(TimeUnit(TimeUnit::BEATS, 19200), 0.25);

	for (double in = 0.0; in < 32; in += 0.23) {
		TimeStamp beats(TimeUnit(TimeUnit::BEATS, 19200), in);

		/*cout << "Q(" << in << ", 1/4) = "
		        << quantize(q, beats) << endl;*/

		if (quantize(q, beats).subticks() % (19200 / 4) != 0) {
			return 1;
		}
	}

	return 0;
}
