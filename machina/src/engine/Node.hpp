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

#ifndef MACHINA_NODE_HPP
#define MACHINA_NODE_HPP

#include <set>

#include "machina/types.hpp"

#include "Action.hpp"
#include "MIDISink.hpp"
#include "Schrodinbit.hpp"
#include "Stateful.hpp"

namespace machina {

class Edge;
using Raul::TimeDuration;
using Raul::TimeStamp;
using Raul::TimeUnit;

/** A node is a state (as in a FSM diagram), or "note".
 *
 * It contains a action, as well as a duration and pointers to its
 * successors (states/nodes that (may) follow it).
 *
 * Initial nodes do not have enter actions (since they are entered at
 * an undefined point in time <= 0).
 */
class Node : public Stateful
{
public:
	Node(TimeDuration duration, bool initial=false);
	Node(const Node& copy);

	bool operator==(const Node& rhs) const;

	void set_enter_action(SPtr<Action> action);
	void set_exit_action(SPtr<Action> action);

	SPtr<const Action> enter_action() const { return _enter_action; }
	SPtr<Action>       enter_action()       { return _enter_action; }
	SPtr<const Action> exit_action()  const { return _exit_action; }
	SPtr<Action>       exit_action()        { return _exit_action; }

	void enter(MIDISink* sink, TimeStamp time);
	void exit(MIDISink* sink, TimeStamp time);

	void edges_changed();

	void       add_edge(SPtr<Edge> edge);
	void       remove_edge(SPtr<Edge> edge);
	SPtr<Edge> remove_edge_to(SPtr<Node> node);
	bool       connected_to(SPtr<Node> node);

	void set(URIInt key, const Atom& value);
	void write_state(Sord::Model& model);

	bool         is_initial() const           { return _is_initial; }
	bool         is_active() const            { return _is_active; }
	TimeStamp    enter_time() const           { return _enter_time; }
	TimeStamp    exit_time() const            { return _enter_time + _duration; }
	TimeDuration duration() const             { return _duration; }
	void         set_duration(TimeDuration d) { _duration = d; }
	bool         is_selector() const          { return _is_selector; }
	void         set_selector(bool i);

	inline bool changed()     { return _changed; }
	inline void set_changed() { _changed = true; }

	struct EdgeHeadOrder {
		inline bool operator()(const SPtr<const Edge>& a,
		                       const SPtr<const Edge>& b) {
			return a.get() < b.get();
		}
	};

	SPtr<Edge> edge_to(SPtr<Node> head) const;

	typedef std::set<SPtr<Edge>, EdgeHeadOrder> Edges;

	Edges& edges() { return _edges; }

	SPtr<Edge> random_edge();

private:
	Node& operator=(const Node& other); // undefined

	TimeStamp    _enter_time;   ///< valid iff _is_active
	TimeDuration _duration;
	SPtr<Action> _enter_action;
	SPtr<Action> _exit_action;
	Edges        _edges;
	Schrodinbit  _changed;
	bool         _is_initial;
	bool         _is_selector;
	bool         _is_active;
};

} // namespace machina

#endif // MACHINA_NODE_HPP
