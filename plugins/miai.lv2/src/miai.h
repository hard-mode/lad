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

#ifndef MIAI_H
#define MIAI_H

#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

typedef struct {
	LV2_URID_Map* map;
} Host;

typedef struct {
	LV2_URID atom_Sequence;
	LV2_URID midi_MidiEvent;
} URIs;

static unsigned
init_host(const LV2_Feature* const* features, Host* host)
{
	for (unsigned i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			host->map = (LV2_URID_Map*)features[i]->data;
			return 1;
		}
	}
	return 0;
}

static void
init_uris(LV2_URID_Map* map, URIs* uris)
{
	uris->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->atom_Sequence  = map->map(map->handle, LV2_ATOM__Sequence);
}

static inline void
clear_sequence(const URIs* uris, LV2_Atom_Sequence* seq)
{
	seq->atom.type = uris->atom_Sequence;
	seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
	seq->body.unit = 0;
	seq->body.pad  = 0;
}

static inline void
append_midi_event(const URIs*          uris,
                  LV2_Atom_Sequence*   seq,
                  uint32_t             time,
                  const uint8_t* const msg,
                  uint32_t             size)
{
	LV2_Atom_Event* end = lv2_atom_sequence_end(&seq->body, seq->atom.size);
	end->time.frames = time;
	end->body.type   = uris->midi_MidiEvent;
	end->body.size   = size;
	memcpy(end + 1, msg, size);
	seq->atom.size += lv2_atom_pad_size(sizeof(LV2_Atom_Event) + size);
}

#endif  // MIAI_H
