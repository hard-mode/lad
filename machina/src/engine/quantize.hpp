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

#ifndef MACHINA_QUANTIZE_HPP
#define MACHINA_QUANTIZE_HPP

#include <cmath>

#include "raul/TimeStamp.hpp"

namespace machina {

inline TimeStamp
quantize(TimeStamp q, TimeStamp t)
{
	assert(q.unit() == t.unit());
	// FIXME: Precision problem?  Should probably stay in discrete domain
	const double qd = q.to_double();
	const double td = t.to_double();
	return TimeStamp(t.unit(), (qd > 0) ? lrint(td / qd) * qd : td);
}

inline double
quantize(double q, double t)
{
	return (q > 0)
	       ? lrint(t / q) * q
		   : t;
}

} // namespace machina

#endif // MACHINA_QUANTIZE_HPP
