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
	PORT_INPUT = 0,
	PORT_SYS   = 1,
	PORT_CH1   = 2,
	PORT_CH16  = 17
};

typedef struct {
	struct {
		const LV2_Atom_Sequence* in;
		LV2_Atom_Sequence*       out[17];
	} ports;

	Host host;
	URIs uris;
} ChanDemux;

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
	ChanDemux* self = (ChanDemux*)instance;

	if (port == PORT_INPUT) {
		self->ports.in = (const LV2_Atom_Sequence*)data;
	} else if (port == PORT_SYS) {
		self->ports.out[0] = (LV2_Atom_Sequence*)data;
	} else if (port <= PORT_CH16) {
		self->ports.out[port - 1] = (LV2_Atom_Sequence*)data;
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

	ChanDemux* self = (ChanDemux*)malloc(sizeof(ChanDemux));
	if (self) {
		init_uris(host.map, &self->uris);
	}

	return (LV2_Handle)self;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	ChanDemux* self = (ChanDemux*)instance;

	for (uint32_t i = 1; i < PORT_CH16; ++i) {
		clear_sequence(&self->uris, self->ports.out[i]);
	}

	LV2_ATOM_SEQUENCE_FOREACH(self->ports.in, ev) {
		if (ev->body.type == self->uris.midi_MidiEvent) {
			const uint8_t* const msg = LV2_ATOM_BODY_CONST(&ev->body);
			if (lv2_midi_is_system_message(msg)) {
				append_midi_event(&self->uris,
				                  self->ports.out[PORT_SYS - 1],
				                  ev->time.frames,
				                  msg,
				                  ev->body.size);
			} else if (lv2_midi_is_voice_message(msg)) {
				const uint8_t chan = msg[0] & 0x0F;
				append_midi_event(&self->uris,
				                  self->ports.out[PORT_CH1 - 1 + chan],
				                  ev->time.frames,
				                  msg,
				                  ev->body.size);
			}
		}
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/miai/chan_demux",
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
