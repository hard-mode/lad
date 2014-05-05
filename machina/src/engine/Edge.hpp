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

#ifndef MACHINA_EDGE_HPP
#define MACHINA_EDGE_HPP

#include <list>

#include "machina/types.hpp"

#include "Action.hpp"
#include "Stateful.hpp"

namespace machina {

class Node;

class Edge : public Stateful
{
public:
	Edge(WPtr<Node> tail, SPtr<Node> head, float probability=1.0f)
		: _tail(tail)
		, _head(head)
		, _probability(probability)
	{}

	void set(URIInt key, const Atom& value);
	void write_state(Sord::Model& model);

	WPtr<Node> tail() { return _tail; }
	SPtr<Node> head() { return _head; }

	void set_tail(WPtr<Node> tail) { _tail = tail; }
	void set_head(SPtr<Node> head) { _head = head; }

	inline float probability() const      { return _probability; }
	inline void  set_probability(float p) { _probability = p; }

private:
	WPtr<Node> _tail;
	SPtr<Node> _head;
	float      _probability;
};

} // namespace machina

#endif // MACHINA_EDGE_HPP
