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

#ifndef MACHINA_URIS_HPP
#define MACHINA_URIS_HPP

#include <stdint.h>

#include "machina/Atom.hpp"
#include "machina/types.hpp"

#define MACHINA_URI_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define MACHINA_NS "http://drobilla.net/ns/machina#"

#define MACHINA_NS_Machine      MACHINA_NS "Machine"
#define MACHINA_NS_Node         MACHINA_NS "Node"
#define MACHINA_NS_SelectorNode MACHINA_NS "SelectorNode"
#define MACHINA_NS_arc          MACHINA_NS "arc"
#define MACHINA_NS_duration     MACHINA_NS "duration"
#define MACHINA_NS_head         MACHINA_NS "head"
#define MACHINA_NS_node         MACHINA_NS "node"
#define MACHINA_NS_onEnter      MACHINA_NS "onEnter"
#define MACHINA_NS_onExit       MACHINA_NS "onExit"
#define MACHINA_NS_probability  MACHINA_NS "probability"
#define MACHINA_NS_start        MACHINA_NS "start"
#define MACHINA_NS_tail         MACHINA_NS "tail"

namespace machina {

class URIs
{
public:
	static void init() { _instance = new URIs(); }

	static inline const URIs& instance() { assert(_instance); return *_instance; }

	URIInt machina_Edge;
	URIInt machina_MidiAction;
	URIInt machina_Node;
	URIInt machina_active;
	URIInt machina_canvas_x;
	URIInt machina_canvas_y;
	URIInt machina_duration;
	URIInt machina_enter_action;
	URIInt machina_exit_action;
	URIInt machina_head_id;
	URIInt machina_initial;
	URIInt machina_note_number;
	URIInt machina_probability;
	URIInt machina_selector;
	URIInt machina_tail_id;
	URIInt rdf_type;

private:
	URIs()
		: machina_Edge(100)
		, machina_MidiAction(101)
		, machina_Node(102)
		, machina_active(1)
		, machina_canvas_x(2)
		, machina_canvas_y(3)
		, machina_duration(4)
		, machina_enter_action(11)
		, machina_exit_action(12)
		, machina_head_id(5)
		, machina_initial(6)
		, machina_note_number(13)
		, machina_probability(7)
		, machina_selector(8)
		, machina_tail_id(9)
		, rdf_type(10)
	{}

	static URIs* _instance;
};

} // namespace machina

#endif // MACHINA_URIS_HPP
