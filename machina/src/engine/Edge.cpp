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

#include "machina/Atom.hpp"
#include "machina/URIs.hpp"
#include "sord/sordmm.hpp"

#include "Edge.hpp"
#include "Node.hpp"

namespace machina {

void
Edge::set(URIInt key, const Atom& value)
{
	if (key == URIs::instance().machina_probability) {
		_probability = value.get<float>();
	}
}

void
Edge::write_state(Sord::Model& model)
{
	using namespace Raul;

	const Sord::Node& rdf_id = this->rdf_id(model.world());

	SPtr<Node> tail = _tail.lock();
	SPtr<Node> head = _head;

	if (!tail || !head)
		return;

	assert(tail->rdf_id(model.world()).is_valid()
	       && head->rdf_id(model.world()).is_valid());

	model.add_statement(rdf_id,
	                    Sord::URI(model.world(), MACHINA_NS_tail),
	                    tail->rdf_id(model.world()));

	model.add_statement(rdf_id,
	                    Sord::URI(model.world(), MACHINA_NS_head),
	                    head->rdf_id(model.world()));

	model.add_statement(
		rdf_id,
		Sord::URI(model.world(), MACHINA_NS_probability),
		Sord::Literal::decimal(model.world(), _probability, 7));
}

} // namespace machina
