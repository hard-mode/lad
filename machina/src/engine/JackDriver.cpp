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

#include <iostream>

#include "machina/Context.hpp"
#include "machina/URIs.hpp"
#include "machina/Updates.hpp"
#include "machina_config.h"

#include "Edge.hpp"
#include "JackDriver.hpp"
#include "LearnRequest.hpp"
#include "MachineBuilder.hpp"
#include "MidiAction.hpp"

using namespace machina;
using namespace std;

namespace machina {

JackDriver::JackDriver(Forge& forge, SPtr<Machine> machine)
	: Driver(forge, machine)
	, _client(NULL)
	, _machine_changed(0)
	, _input_port(NULL)
	, _output_port(NULL)
	, _context(forge, 48000, MACHINA_PPQN, 120.0)
	, _frames_unit(TimeUnit::FRAMES, 48000)
	, _beats_unit(TimeUnit::BEATS, 19200)
	, _stop(0)
	, _stop_flag(false)
	, _record_dur(_frames_unit) // = 0
	, _is_activated(false)
{
	_context.set_sink(this);
}

JackDriver::~JackDriver()
{
	detach();
}

void
JackDriver::attach(const std::string& client_name)
{
	// Already connected
	if (_client) {
		return;
	}

	jack_set_error_function(jack_error_cb);

	_client = jack_client_open(client_name.c_str(), JackNullOption, NULL, NULL);

	if (_client == NULL) {
		_is_activated = false;
	} else {
		jack_set_error_function(jack_error_cb);
		jack_on_shutdown(_client, jack_shutdown_cb, this);
		jack_set_process_callback(_client, jack_process_cb, this);
	}

	if (jack_client()) {

		_context.time().set_tick_rate(sample_rate());

		_input_port = jack_port_register(jack_client(),
		                                 "in",
		                                 JACK_DEFAULT_MIDI_TYPE,
		                                 JackPortIsInput,
		                                 0);

		if (!_input_port) {
			std::cerr << "WARNING: Failed to create MIDI input port."
			          << std::endl;
		}

		_output_port = jack_port_register(jack_client(),
		                                  "out",
		                                  JACK_DEFAULT_MIDI_TYPE,
		                                  JackPortIsOutput,
		                                  0);

		if (!_output_port) {
			std::cerr << "WARNING: Failed to create MIDI output port."
			          << std::endl;
		}

		if (!_machine) {
			_machine = SPtr<Machine>(
				new Machine(TimeUnit::frames(
					            jack_get_sample_rate(jack_client()))));
		}
	}
}

void
JackDriver::detach()
{
	if (_is_activated) {
		_is_activated = false;
		_stop.timed_wait(1000);
	}

	if (_input_port) {
		jack_port_unregister(jack_client(), _input_port);
		_input_port = NULL;
	}

	if (_output_port) {
		jack_port_unregister(jack_client(), _output_port);
		_output_port = NULL;
	}

	if (_client) {
		deactivate();
		jack_client_close(_client);
		_client       = NULL;
		_is_activated = false;
	}
}

void
JackDriver::activate()
{
	if (!jack_activate(_client)) {
		_is_activated = true;
	} else {
		_is_activated = false;
	}
}

void
JackDriver::deactivate()
{
	if (_client) {
		jack_deactivate(_client);
	}

	_is_activated = false;
}

void
JackDriver::set_machine(SPtr<Machine> machine)
{
	if (machine == _machine) {
		return;
	}

	SPtr<Machine> last_machine = _last_machine; // Keep a reference
	_machine_changed.reset(0);
	assert(!last_machine.unique());
	_machine = machine;
	if (is_activated()) {
		_machine_changed.wait();
	}
	assert(_machine == machine);
	last_machine.reset();
}

void
JackDriver::read_input_recording(SPtr<Machine>          machine,
                                 const Raul::TimeSlice& time)
{
	const jack_nframes_t nframes  = time.length_ticks().ticks();
	void*                buf      = jack_port_get_buffer(_input_port, nframes);
	const jack_nframes_t n_events = jack_midi_get_event_count(buf);

	for (jack_nframes_t i = 0; i < n_events; ++i) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, buf, i);

		const TimeStamp rel_time_frames = TimeStamp(_frames_unit, ev.time);
		const TimeStamp time_frames     = _record_dur + rel_time_frames;
		_recorder->write(time.ticks_to_beats(time_frames), ev.size, ev.buffer);
	}

	if (n_events > 0) {
		_recorder->whip();
	}

	_record_dur += time.length_ticks();
}

void
JackDriver::read_input_playing(SPtr<Machine>          machine,
                               const Raul::TimeSlice& time)
{
	const jack_nframes_t nframes  = time.length_ticks().ticks();
	void*                buf      = jack_port_get_buffer(_input_port, nframes);
	const jack_nframes_t n_events = jack_midi_get_event_count(buf);

	for (jack_nframes_t i = 0; i < n_events; ++i) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, buf, i);

		if (ev.buffer[0] == 0x90) {
			const SPtr<LearnRequest> learn = machine->pending_learn();
			if (learn) {
				learn->enter_action()->set_event(ev.size, ev.buffer);
				learn->start(_quantization,
				             TimeStamp(TimeUnit::frames(sample_rate()),
				                       jack_last_frame_time(_client)
				                       + ev.time, 0));
			}

		} else if (ev.buffer[0] == 0x80) {
			const SPtr<LearnRequest> learn = machine->pending_learn();
			if (learn && learn->started()) {
				learn->exit_action()->set_event(ev.size, ev.buffer);
				learn->finish(TimeStamp(TimeUnit::frames(sample_rate()),
				                        jack_last_frame_time(_client) + ev.time,
				                        0));

				const uint64_t id = Stateful::next_id();
				write_set(_updates, id,
				          URIs::instance().rdf_type,
				          _forge.make_urid(URIs::instance().machina_MidiAction));
				write_set(_updates, learn->node()->id(),
				          URIs::instance().machina_enter_action,
				          _forge.make((int32_t)id));
				write_set(_updates, id,
				          URIs::instance().machina_note_number,
				          _forge.make((int32_t)ev.buffer[1]));

				machine->clear_pending_learn();
			}
		}
	}
}

void
JackDriver::write_event(Raul::TimeStamp time,
                        size_t          size,
                        const byte*     event)
{
	if (!_output_port) {
		return;
	}

	const Raul::TimeSlice& slice = _context.time();

	if (slice.beats_to_ticks(time) + slice.offset_ticks() <
	    slice.start_ticks()) {
		std::cerr << "ERROR: Missed event by "
		          << (slice.start_ticks()
		              - slice.beats_to_ticks(time)
		              + slice.offset_ticks())
		          << " ticks"
		          << "\n\tbpm: " << slice.bpm()
		          << "\n\tev time: " << slice.beats_to_ticks(time)
		          << "\n\tcycle_start: " << slice.start_ticks()
		          << "\n\tcycle_end: " << (slice.start_ticks()
		                                   + slice.length_ticks())
		          << "\n\tcycle_length: " << slice.length_ticks()
		          << std::endl << std::endl;
		return;
	}

	const TimeDuration nframes = slice.length_ticks();
	const TimeStamp    offset  = slice.beats_to_ticks(time)
	    + slice.offset_ticks() - slice.start_ticks();

	if (!(offset < slice.offset_ticks() + nframes)) {
		std::cerr << "ERROR: Event offset " << offset << " outside cycle "
		          << "\n\tbpm: " << slice.bpm()
		          << "\n\tev time: " << slice.beats_to_ticks(time)
		          << "\n\tcycle_start: " << slice.start_ticks()
		          << "\n\tcycle_end: " << slice.start_ticks()
		+ slice.length_ticks()
		          << "\n\tcycle_length: " << slice.length_ticks() << std::endl;
	} else {
#ifdef JACK_MIDI_NEEDS_NFRAMES
		jack_midi_event_write(
		    jack_port_get_buffer(_output_port, nframes), offset,
		    event, size, nframes);
#else
		jack_midi_event_write(
		    jack_port_get_buffer(_output_port, nframes.ticks()), offset.ticks(),
		    event, size);
#endif
	}
}

void
JackDriver::on_process(jack_nframes_t nframes)
{
	if (!_is_activated) {
		_stop.post();
		return;
	}

	_context.time().set_bpm(_bpm);

	assert(_output_port);
	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes));

	TimeStamp length_ticks(TimeStamp(_context.time().ticks_unit(), nframes));

	_context.time().set_length(length_ticks);
	_context.time().set_offset(TimeStamp(_context.time().ticks_unit(), 0, 0));

	/* Take a reference to machine here and use only it during the process
	 * cycle so _machine can be switched with set_machine during a cycle. */
	SPtr<Machine> machine = _machine;

	// Machine was switched since last cycle, finalize old machine.
	if (machine != _last_machine) {
		if (_last_machine) {
			assert(!_last_machine.unique());                              // Realtime, can't delete
			_last_machine->reset(_context.sink(), _last_machine->time()); // Exit all active states
			_last_machine.reset();                                        // Cut our reference
		}
		_machine_changed.post(); // Signal we're done with it
	}

	if (!machine) {
		_last_machine = machine;
		goto end;
	}

	if (_stop_flag) {
		machine->reset(_context.sink(), _context.time().start_beats());
	}

	switch (_play_state) {
	case PlayState::STOPPED:
		break;
	case PlayState::PLAYING:
		read_input_playing(machine, _context.time());
		break;
	case PlayState::RECORDING:
	case PlayState::STEP_RECORDING:
		read_input_recording(machine, _context.time());
		break;
	}

	if (machine->is_empty()) {
		goto end;
	}

	while (true) {
		const uint32_t run_dur_frames = machine->run(_context, _updates);

		if (run_dur_frames == 0) {
			// Machine didn't run at all (machine has no initial states)
			machine->reset(_context.sink(), machine->time()); // Try again next cycle
			_context.time().set_slice(TimeStamp(_frames_unit, 0, 0),
			                          TimeStamp(_frames_unit, 0, 0));
			break;

		} else if (machine->is_finished()) {
			// Machine ran for portion of cycle and is finished
			machine->reset(_context.sink(), machine->time());

			_context.time().set_slice(TimeStamp(_frames_unit, 0, 0),
			                          TimeStamp(_frames_unit, nframes
			                                    - run_dur_frames, 0));
			_context.time().set_offset(TimeStamp(_frames_unit, run_dur_frames,
			                                     0));

		} else {
			// Machine ran for entire cycle
			_context.time().set_slice(
			    _context.time().start_ticks() + _context.time().length_ticks(),
			    TimeStamp(_frames_unit, 0, 0));
			break;
		}
	}

end:
	/* Remember the last machine run, in case a switch happens and
	 * we need to finalize it next cycle. */
	_last_machine = machine;

	if (_stop_flag) {
		_context.time().set_slice(TimeStamp(_frames_unit, 0, 0),
		                          TimeStamp(_frames_unit, 0, 0));
		_stop_flag = false;
		_stop.post();
	}
}

void
JackDriver::set_play_state(PlayState state)
{
	switch (state) {
	case PlayState::STOPPED:
		switch (_play_state) {
		case PlayState::STOPPED:
			break;
		case PlayState::RECORDING:
		case PlayState::STEP_RECORDING:
			finish_record();
			// nobreak
		case PlayState::PLAYING:
			_stop_flag = true;
			_stop.wait();
		}
		break;
	case PlayState::RECORDING:
		start_record(false);
		break;
	case PlayState::STEP_RECORDING:
		start_record(true);
		break;
	case PlayState::PLAYING:
		if (_play_state == PlayState::RECORDING ||
		    _play_state == PlayState::STEP_RECORDING) {
			finish_record();
		}
	}
	Driver::set_play_state(state);
}

void
JackDriver::start_record(bool step)
{
	const double q = (step || _quantize_record) ? _quantization : 0.0;
	switch (_play_state) {
	case PlayState::STOPPED:
	case PlayState::PLAYING:
		_recorder = SPtr<Recorder>(
			new Recorder(_forge, 1024, _beats_unit, q, step));
		_record_dur = 0;
		break;
	case PlayState::RECORDING:
	case PlayState::STEP_RECORDING:
		_recorder->builder()->set_step(true);
		break;
	}
	_play_state = step ? PlayState::STEP_RECORDING : PlayState::RECORDING;
}

void
JackDriver::finish_record()
{
	_play_state = PlayState::PLAYING;
	SPtr<Machine> machine = _recorder->finish();
	_recorder.reset();
	_machine->merge(*machine.get());
}

int
JackDriver::jack_process_cb(jack_nframes_t nframes, void* jack_driver)
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	me->on_process(nframes);
	return 0;
}

void
JackDriver::jack_shutdown_cb(void* jack_driver)
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	me->_client = NULL;
}

void
JackDriver::jack_error_cb(const char* msg)
{
	cerr << "[JACK] Error: " << msg << endl;
}

} // namespace machina
