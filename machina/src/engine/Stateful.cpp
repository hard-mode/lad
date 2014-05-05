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

#include "Stateful.hpp"

namespace machina {

uint64_t Stateful::_next_id = 1;

Stateful::Stateful()
	: _id(next_id())
{}

const Sord::Node&
Stateful::rdf_id(Sord::World& world) const
{
	if (!_rdf_id.is_valid()) {
		std::ostringstream ss;
		ss << "b" << _id;
		_rdf_id = Sord::Node(world, Sord::Node::BLANK, ss.str());
	}

	return _rdf_id;
}

} // namespace machina
