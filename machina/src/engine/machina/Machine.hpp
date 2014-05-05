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

#ifndef MACHINA_MACHINE_HPP
#define MACHINA_MACHINE_HPP

#include <vector>
#include <set>

#include "machina/types.hpp"
#include "machina/Atom.hpp"
#include "raul/RingBuffer.hpp"
#include "raul/TimeSlice.hpp"
#include "sord/sordmm.hpp"

#include "types.hpp"
#include "Node.hpp"

namespace machina {

class Context;
class LearnRequest;

/** A (Finite State) Machine.
 */
class Machine : public Stateful
{
public:
	explicit Machine(TimeUnit unit);

	/** Copy a Machine.
	 *
	 * Creates a deep copy which is the 'same' machine, but with fresh state,
	 * i.e. all nodes are inactive and time is at zero.
	 */
	Machine(const Machine& copy);

	/** Completely replace this machine's contents with a deep copy. */
	Machine& operator=(const Machine& copy);

	bool operator==(const Machine& rhs) const;

	/** Merge another machine into this machine. */
	void merge(const Machine& machine);

	bool is_empty()    { return _nodes.empty(); }
	bool is_finished() { return _is_finished; }

	void add_node(SPtr<Node> node);
	void remove_node(SPtr<Node> node);
	void learn(SPtr<Raul::Maid> maid, SPtr<Node> node);

	void write_state(Sord::Model& model);

	/** Exit all active nodes and reset time to 0. */
	void reset(MIDISink* sink, Raul::TimeStamp time);

	/** Run the machine for a (real) time slice.
	 *
	 * Returns the duration of time the machine actually ran in frames.
	 *
	 * Caller can check is_finished() to determine if the machine still has any
	 * active nodes.  If not, time() will return the exact time stamp the
	 * machine actually finished on (so it can be restarted immediately
	 * with sample accuracy if necessary).
	 */
	uint32_t run(Context& context, SPtr<Raul::RingBuffer> updates);

	// Any context
	inline Raul::TimeStamp time() const { return _time; }

	SPtr<LearnRequest> pending_learn()       { return _pending_learn; }
	void               clear_pending_learn() { _pending_learn.reset(); }

	typedef std::set< SPtr<Node> > Nodes;
	Nodes&       nodes()       { return _nodes; }
	const Nodes& nodes() const { return _nodes; }

	SPtr<Node> initial_node() const { return _initial_node; }

	SPtr<Node> random_node();
	SPtr<Edge> random_edge();

	float fitness;  // For GA

private:
	/** Return the active Node with the earliest exit time. */
	SPtr<Node> earliest_node() const;

	void assign(const Machine& other);

	/** Enter a node at the current time (called by run()).
	 *
	 * @return true if node was entered, otherwise voics are exhausted.
	 */
	bool enter_node(Context&               context,
	                SPtr<Node>             node,
	                SPtr<Raul::RingBuffer> updates);

	/** Exit a node at the current time (called by run()). */
	void exit_node(Context&               context,
	               SPtr<Node>             node,
	               SPtr<Raul::RingBuffer> updates);

	static const size_t MAX_ACTIVE_NODES = 128;

	SPtr<Node>                _initial_node;
	std::vector< SPtr<Node> > _active_nodes;

	SPtr<LearnRequest> _pending_learn;
	Nodes              _nodes;
	Raul::TimeStamp    _time;

	bool               _is_finished;
};

} // namespace machina

#endif // MACHINA_MACHINE_HPP
