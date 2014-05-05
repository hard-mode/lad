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

#ifndef MACHINA_DRIVER_HPP
#define MACHINA_DRIVER_HPP

#include "raul/RingBuffer.hpp"

#include "machina/types.hpp"

#include "MIDISink.hpp"

namespace machina {

class Machine;

class Driver : public MIDISink
{
public:
	Driver(Forge& forge, SPtr<Machine> machine)
		: _forge(forge)
		, _machine(machine)
		, _play_state(PlayState::STOPPED)
		, _bpm(120.0)
		, _quantization(0.125)
		, _quantize_record(0)
	{}

	enum class PlayState {
		STOPPED,
		PLAYING,
		RECORDING,
		STEP_RECORDING
	};

	virtual ~Driver() {}

	SPtr<Machine> machine() { return _machine; }

	virtual void set_machine(SPtr<Machine> machine) {
		_machine = machine;
	}

	SPtr<Raul::RingBuffer> update_sink() { return _updates; }

	void set_update_sink(SPtr<Raul::RingBuffer> b) {
		_updates = b;
	}

	virtual void set_bpm(double bpm)             { _bpm = bpm; }
	virtual void set_quantization(double q)      { _quantization = q; }
	virtual void set_quantize_record(bool q)     { _quantize_record = q; }
	virtual void set_play_state(PlayState state) { _play_state = state; }

	virtual bool is_activated() const { return false; }
	virtual void activate()           {}
	virtual void deactivate()         {}

	PlayState play_state() const { return _play_state; }

protected:
	Forge&                 _forge;
	SPtr<Machine>          _machine;
	SPtr<Raul::RingBuffer> _updates;
	PlayState              _play_state;
	double                 _bpm;
	double                 _quantization;
	bool                   _quantize_record;
};

} // namespace machina

#endif // MACHINA_JACKDRIVER_HPP
