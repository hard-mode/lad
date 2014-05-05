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
#include <list>

#include <glibmm/convert.h>

#include "machina/Context.hpp"
#include "machina/Machine.hpp"
#include "machina/types.hpp"

#include "Edge.hpp"
#include "SMFDriver.hpp"
#include "SMFReader.hpp"
#include "SMFWriter.hpp"
#include "quantize.hpp"

using namespace std;

namespace machina {

SMFDriver::SMFDriver(Forge& forge, Raul::TimeUnit unit)
	: Driver(forge, SPtr<Machine>())
{
	_writer = SPtr<SMFWriter>(new SMFWriter(unit));
}

/** Learn a single track from the MIDI file at @a uri
 *
 * @track selects which track of the MIDI file to import, starting from 1.
 *
 * Currently only file:// URIs are supported.
 * @return the resulting machine.
 */
SPtr<Machine>
SMFDriver::learn(const string&      filename,
                 unsigned           track,
                 double             q,
                 Raul::TimeDuration max_duration)
{
	//assert(q.unit() == max_duration.unit());
	SPtr<Machine>        m(new Machine(max_duration.unit()));
	SPtr<MachineBuilder> builder = SPtr<MachineBuilder>(
		new MachineBuilder(m, q, false));
	SMFReader reader;

	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SPtr<Machine>();
	}

	if (track > reader.num_tracks()) {
		return SPtr<Machine>();
	} else {
		learn_track(builder, reader, track, q, max_duration);
	}

	m->reset(NULL, m->time());

	if (m->nodes().size() > 1) {
		return m;
	} else {
		return SPtr<Machine>();
	}
}

/** Learn all tracks from a MIDI file into a single machine.
 *
 * This will result in one disjoint subgraph in the machine for each track.
 */
SPtr<Machine>
SMFDriver::learn(const string& filename, double q, Raul::TimeStamp max_duration)
{
	SPtr<Machine>        m(new Machine(max_duration.unit()));
	SPtr<MachineBuilder> builder = SPtr<MachineBuilder>(
		new MachineBuilder(m, q, false));
	SMFReader reader;

	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SPtr<Machine>();
	}

	for (unsigned t = 1; t <= reader.num_tracks(); ++t) {
		builder->reset();
		learn_track(builder, reader, t, q, max_duration);
	}

	m->reset(NULL, m->time());

	if (m->nodes().size() > 1) {
		return m;
	} else {
		return SPtr<Machine>();
	}
}

void
SMFDriver::learn_track(SPtr<MachineBuilder> builder,
                       SMFReader&           reader,
                       unsigned             track,
                       double               q,
                       Raul::TimeDuration   max_duration)
{
	const bool found_track = reader.seek_to_track(track);
	if (!found_track) {
		return;
	}

	uint8_t  buf[4];
	uint32_t ev_size;
	uint32_t ev_delta_time;

	Raul::TimeUnit unit = Raul::TimeUnit(TimeUnit::BEATS, MACHINA_PPQN);

	uint64_t t = 0;
	while (reader.read_event(4, buf, &ev_size, &ev_delta_time) >= 0) {
		t += ev_delta_time;

		const uint32_t beats     = t / (uint32_t)reader.ppqn();
		const uint32_t smf_ticks = t % (uint32_t)reader.ppqn();
		const double   frac      = smf_ticks / (double)reader.ppqn();
		const uint32_t ticks     = frac * MACHINA_PPQN;

		if (!max_duration.is_zero() && t > max_duration.to_double()) {
			break;
		}

		if (ev_size > 0) {
			// TODO: quantize
			builder->event(TimeStamp(unit, beats, ticks), ev_size, buf);
		}
	}

	builder->resolve();
}

void
SMFDriver::run(SPtr<Machine> machine, Raul::TimeStamp max_time)
{
	// FIXME: unit kludge (tempo only)
	Context context(_forge, machine->time().unit().ppt(),
	                _writer->unit().ppt(), 120.0);
	context.set_sink(this);
	context.time().set_slice(TimeStamp(max_time.unit(), 0, 0),
	                         context.time().beats_to_ticks(max_time));
	machine->run(context, SPtr<Raul::RingBuffer>());
}

} // namespace machina
