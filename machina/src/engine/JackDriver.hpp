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

#ifndef MACHINA_JACKDRIVER_HPP
#define MACHINA_JACKDRIVER_HPP

#include <jack/jack.h>
#include <jack/midiport.h>

#include "raul/Semaphore.hpp"

#include "machina/Context.hpp"
#include "machina/Driver.hpp"
#include "machina/Machine.hpp"
#include "machina/types.hpp"

#include "Recorder.hpp"

namespace machina {

class MidiAction;
class Node;

/** Realtime JACK Driver.
 *
 * "Ticks" are individual frames when running under this driver, and all code
 * in the processing context must be realtime safe (non-blocking).
 */
class JackDriver : public machina::Driver
{
public:
	JackDriver(Forge&        forge,
	           SPtr<Machine> machine = SPtr<Machine>());
	~JackDriver();

	void attach(const std::string& client_name);
	void detach();

	void activate();
	void deactivate();

	void set_machine(SPtr<Machine> machine);

	void write_event(Raul::TimeStamp      time,
	                 size_t               size,
	                 const unsigned char* event);

	void set_play_state(PlayState state);

	void start_transport() { jack_transport_start(_client); }
	void stop_transport()  { jack_transport_stop(_client); }

	void rewind_transport() {
		jack_position_t zero;
		zero.frame = 0;
		zero.valid = (jack_position_bits_t)0;
		jack_transport_reposition(_client, &zero);
	}

	bool is_activated() const { return _is_activated; }
	bool is_attached()  const { return _client != NULL;  }
	bool is_realtime()  const { return _client && jack_is_realtime(_client); }

	jack_nframes_t sample_rate() const { return jack_get_sample_rate(_client); }
	jack_client_t* jack_client() const { return _client; }

private:
	void read_input_recording(SPtr<Machine>          machine,
	                          const Raul::TimeSlice& time);

	void read_input_playing(SPtr<Machine>          machine,
	                        const Raul::TimeSlice& time);

	static void jack_error_cb(const char* msg);
	static int  jack_process_cb(jack_nframes_t nframes, void* me);
	static void jack_shutdown_cb(void* me);

	void start_record(bool step);
	void finish_record();

	void on_process(jack_nframes_t nframes);

	jack_client_t* _client;

	Raul::Semaphore _machine_changed;
	SPtr<Machine>   _last_machine;

	jack_port_t* _input_port;
	jack_port_t* _output_port;

	Context _context;

	Raul::TimeUnit _frames_unit;
	Raul::TimeUnit _beats_unit;

	Raul::Semaphore _stop;
	bool            _stop_flag;

	Raul::TimeDuration _record_dur;
	SPtr<Recorder>     _recorder;
	bool               _is_activated;
};

} // namespace machina

#endif // MACHINA_JACKDRIVER_HPP
