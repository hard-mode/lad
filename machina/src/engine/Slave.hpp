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

#ifndef MACHINA_SLAVE_HPP
#define MACHINA_SLAVE_HPP

#include <thread>

#include "raul/Semaphore.hpp"

namespace machina {

/** Thread driven by (realtime safe) signals.
 *
 * Use this to perform some task in a separate thread you want to 'drive'
 * from a realtime (or otherwise) thread.
 */
class Slave
{
public:
	Slave()
		: _whip(0)
		, _exit_flag(false)
		, _thread(&Slave::_run, this)
	{}

	virtual ~Slave() {
		_exit_flag = true;
		_whip.post();
		_thread.join();
	}

	/** Tell the slave to do whatever work it does.  Realtime safe. */
	inline void whip() { _whip.post(); }

protected:
	/** Worker method.
	 *
	 * This is called once from this thread every time whip() is called.
	 * Implementations likely want to put a single (non loop) chunk of code
	 * here, e.g. to process an event.
	 */
	virtual void _whipped() = 0;

	Raul::Semaphore _whip;

private:
	inline void _run() {
		while (_whip.wait() && !_exit_flag) {
			_whipped();
		}
	}

	bool        _exit_flag;
	std::thread _thread;
};

} // namespace machina

#endif // MACHINA_SLAVE_HPP
