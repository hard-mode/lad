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

#ifndef MACHINA_LEARNREQUEST_HPP
#define MACHINA_LEARNREQUEST_HPP

#include "raul/Maid.hpp"

#include "machina/types.hpp"

#include "MidiAction.hpp"
#include "Node.hpp"

namespace machina {

class Node;
class MidiAction;

/** A request to MIDI learn a certain node.
 */
class LearnRequest : public Raul::Maid::Manageable
{
public:
	static SPtr<LearnRequest> create(SPtr<Raul::Maid> maid, SPtr<Node> node);

	void start(double q, Raul::TimeStamp time);
	void finish(TimeStamp time);

	inline bool started() const { return _started; }

	const SPtr<Node>&       node()         { return _node; }
	const SPtr<MidiAction>& enter_action() { return _enter_action; }
	const SPtr<MidiAction>& exit_action()  { return _exit_action; }

private:
	LearnRequest(SPtr<Raul::Maid> maid, SPtr<Node> node);

	bool                  _started;
	TimeStamp             _start_time;
	double                _quantization;
	SPtr<Node>       _node;
	SPtr<MidiAction> _enter_action;
	SPtr<MidiAction> _exit_action;
};

} // namespace machina

#endif // MACHINA_LEARNREQUEST_HPP
