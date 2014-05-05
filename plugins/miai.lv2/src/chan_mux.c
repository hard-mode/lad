/*
  This file is part of Miai.
  Copyright 2013 David Robillard <http://drobilla.net>

  Miai is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Miai is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Miai.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "miai.h"

enum {
	PORT_SYS  = 0,
	PORT_CH1  = 1,
	PORT_CH16 = 16,
	PORT_OUT  = 17
};

typedef struct {
	struct {
		const LV2_Atom_Sequence* in[17];
		LV2_Atom_Sequence*       out;
	} ports;

	Host host;
	URIs uris;
} ChanMux;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	ChanMux* self = (ChanMux*)instance;

	if (port <= PORT_CH16) {
		self->ports.in[port] = (const LV2_Atom_Sequence*)data;
	} else if (port == PORT_OUT) {
		self->ports.out = (LV2_Atom_Sequence*)data;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Host host;
	if (init_host(features, &host) == 0) {
		return NULL;
	}

	ChanMux* self = (ChanMux*)malloc(sizeof(ChanMux));
	if (self) {
		init_uris(host.map, &self->uris);
	}

	return (LV2_Handle)self;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	ChanMux* self = (ChanMux*)instance;

	clear_sequence(&self->uris, self->ports.out);

	LV2_Atom_Event* iters[17];
	for (uint32_t i = 0; i < 17; ++i) {
		iters[i] = lv2_atom_sequence_begin(&self->ports.in[i]->body);
	}

	while (true) {
		uint32_t        first_index = 0;
		LV2_Atom_Event* first       = NULL;
		for (uint32_t i = 0; i < 17; ++i) {
			const LV2_Atom_Sequence* seq = self->ports.in[i];
			if (!lv2_atom_sequence_is_end(&seq->body, seq->atom.size, iters[i])
			    && (!first || first->time.frames > iters[i]->time.frames)) {
				first       = iters[i];
				first_index = i;
			}
		}

		if (!first || lv2_atom_sequence_is_end(
			    &self->ports.in[first_index]->body,
			    self->ports.in[first_index]->atom.size,
			    first)) {
			break;
		}

		const uint8_t* const first_msg = (const uint8_t*)(&first->body + 1);
		if (first_index == PORT_SYS) {
			if (lv2_midi_is_system_message(first_msg)) {
				append_midi_event(&self->uris,
				                  self->ports.out,
				                  first->time.frames,
				                  (const uint8_t*)(&first->body + 1),
				                  first->body.size);
			}
		} else if (lv2_midi_is_voice_message(first_msg)) {
			/* Voice messages are only 3 bytes, but use more to support 14-bit
			   controller, RPN, and NRPN messages in a single event. */
			uint8_t mapped[16];
			memcpy(mapped, first_msg, sizeof(mapped));
			mapped[0] = (first_msg[0] & 0xF0) + first_index - 1;
			append_midi_event(&self->uris,
			                  self->ports.out,
			                  first->time.frames,
			                  mapped,
			                  first->body.size);
		}
		iters[first_index] = lv2_atom_sequence_next(first);
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/miai/chan_mux",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	return (index == 0) ? &descriptor : NULL;
}
