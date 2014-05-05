/*
  This file is part of Matriseq.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Matriseq is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Matriseq is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Matriseq.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "naub/naub.h"
#include "zix/thread.h"
#include "zix/ring.h"

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define MATRISEQ_URI "http://drobilla.net/plugins/matriseq"

#define GRID_H    8
#define GRID_W    8
#define SEQ_H     (8 * 8)
#define NOTE_MIN  28
#define STEP_TYPE 16
#define RING_SIZE 4096

typedef enum {
	MATRISEQ_IN  = 0,
	MATRISEQ_OUT = 1
} PortIndex;

// URIDs used by this plugin
typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Float;
	LV2_URID log_Error;
	LV2_URID midi_MidiEvent;
	LV2_URID time_Position;
	LV2_URID time_barBeat;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_speed;
} MatriseqURIs;

typedef struct {
	// Port buffers
	LV2_Atom_Sequence* in;
	LV2_Atom_Sequence* out;

	// Features
	LV2_URID_Map* map;
	LV2_Log_Log*  log;

	// LV2 stuff
	LV2_Atom_Forge forge;
	MatriseqURIs   uris;

	// USB stuff
	NaubWorld* naub;
	ZixRing*   ring;
	ZixThread  thread;
	bool       exit;

	// State
	double   rate;
	float    bpm;
	float    speed;
	uint32_t beats_per_bar;
	uint32_t time_frames;
	uint32_t step;
	uint8_t  page_x;
	uint8_t  page_y;
	uint32_t seq[SEQ_H][STEP_TYPE];
} Matriseq;

// Log a message to the host if available, or stderr otherwise.
LV2_LOG_FUNC(3, 4)
static void
print(Matriseq* self, LV2_URID type, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (self->log) {
		self->log->vprintf(self->log->handle, type, fmt, args);
	} else {
		vfprintf(stderr, fmt, args);
	}
	va_end(args);
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Matriseq* self = (Matriseq*)calloc(1, sizeof(Matriseq));
	if (!self) {
		return NULL;
	}

	// Get features
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}
	if (!self->map) {
		print(self, self->uris.log_Error, "Missing feature urid:map.\n");
		free(self);
		return NULL;
	}

	// Initialise LV2 stuff
	LV2_URID_Map* map = self->map;
	self->uris.atom_Blank          = map->map(map->handle, LV2_ATOM__Blank);
	self->uris.atom_Float          = map->map(map->handle, LV2_ATOM__Float);
	self->uris.log_Error           = map->map(map->handle, LV2_LOG__Error);
	self->uris.midi_MidiEvent      = map->map(map->handle, LV2_MIDI__MidiEvent);
	self->uris.time_Position       = map->map(map->handle, LV2_TIME__Position);
	self->uris.time_barBeat        = map->map(map->handle, LV2_TIME__barBeat);
	self->uris.time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
	self->uris.time_speed          = map->map(map->handle, LV2_TIME__speed);
	lv2_atom_forge_init(&self->forge, self->map);

	// Initialise USB stuff
	self->naub = NULL;
	self->ring = zix_ring_new(RING_SIZE);

	// Initialise state
	self->rate          = rate;
	self->bpm           = 140.0f;
	self->speed         = 0.0f;
	self->beats_per_bar = 4;
	self->page_y        = 1;  // Start at note 36 (kick)

	zix_ring_mlock(self->ring);

	return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Matriseq* self = (Matriseq*)instance;

	switch ((PortIndex)port) {
	case MATRISEQ_IN:
		self->in = (LV2_Atom_Sequence*)data;
		break;
	case MATRISEQ_OUT:
		self->out = (LV2_Atom_Sequence*)data;
		break;
	}
}

static uint32_t*
get_cell(Matriseq* self, NaubControlID control)
{
	const uint32_t x = (self->page_x * GRID_W) + control.x;
	const uint32_t y = (self->page_y * GRID_H) + (7 - control.y);
	return &self->seq[y][x];
}

static void
set_button(Matriseq* self, NaubControlID control, bool active)
{
	int32_t value = 0;
	if (*get_cell(self, control)) {
		value = active ? naub_rgb(1, 0, 0) : naub_rgb(1, 1, 0);
	} else {
		value = active ? naub_rgb(0, 0.4, 0) : naub_rgb(0, 0, 0);
	}
	naub_set_control(self->naub, control, value);
}

static void
set_column(Matriseq* self, uint32_t step, bool active)
{
	for (int y = 0; y < 8; ++y) {
		const NaubControlID control = { 0, 0, step % GRID_W, y };
		set_button(self, control, active);
	}
}

static void
set_page_indicators(Matriseq* self)
{
	const NaubControlID page_x_but = { 0, 1, self->page_x, 0 };
	const NaubControlID page_y_but = { 0, 2, 0, 7 - self->page_y };
	naub_set_control(self->naub, page_x_but, naub_rgb(0, 1, 0));
	naub_set_control(self->naub, page_y_but, naub_rgb(0, 1, 0));
}

static void
show_page(Matriseq* self)
{
	for (uint32_t y = 0; y < 8; ++y) {
		for (uint32_t x = 0; x < 8; ++x) {
			const NaubControlID control = { 0, 0, x, y };
			set_button(self, control, x == self->step);
		}
	}
}

static void
pad_event(void* instance, const NaubEvent* event)
{
	Matriseq* self = (Matriseq*)instance;
	if (event->type != NAUB_EVENT_BUTTON) {  // Odd...
		return;
	}

	const NaubControlID control = event->button.control;

	if (control.group == 1 && event->button.pressed) {
		const NaubControlID old_page_x_but = { 0, 1, self->page_x, 0 };
		const NaubControlID old_page_y_but = { 0, 2, 0, 7 - self->page_y };
		if (control.x == 0 && self->page_y < 7) {
			++self->page_y;
		} else if (control.x == 1 && self->page_y > 0) {
			--self->page_y;
		} else if (control.x == 2 && self->page_x > 0) {
			--self->page_x;
		} else if (control.x == 3 && self->page_x < 1) {
			++self->page_x;
		} else {
			return;
		}

		// Turn off old page indicator buttons
		naub_set_control(self->naub, old_page_x_but, naub_rgb(0, 0, 0));
		naub_set_control(self->naub, old_page_y_but, naub_rgb(0, 0, 0));

		// Turn on new page indicator buttons
		set_page_indicators(self);

		// Update grid display
		show_page(self);
	} else if (control.group == 0) {
		if (event->button.pressed) {
			naub_set_control(self->naub, control, naub_rgb(1, 0, 0));
		} else {
			uint32_t* cell = get_cell(self, control);
			*cell = *cell ? 0 : 1;
			set_button(self, control, self->step == control.y);
		}
	}

	naub_flush(self->naub);
}

static void*
pad_thread(void* instance)
{
	Matriseq* self = (Matriseq*)instance;
	uint32_t  step = self->step;

	// Initialise pad
	set_page_indicators(self);
	set_column(self, step, true);
	naub_flush(self->naub);

	while (!naub_handle_events_timeout(self->naub, 10) && !self->exit) {
		uint32_t new_step;
		if (zix_ring_read_space(self->ring) >= sizeof(new_step)) {
			zix_ring_read(self->ring, &new_step, sizeof(new_step));

			const uint32_t begin = self->page_x * GRID_W;
			const uint32_t end   = (self->page_x + 1) * GRID_W;

			// De-highlight old active row
			if (step >= begin && step < end) {
				set_column(self, step, false);
			}

			// Highlight new active row
			if (new_step >= begin && new_step < end) {
				set_column(self, new_step, true);
			}

			// Send bulk update to device
			naub_flush(self->naub);

			step = new_step;
		}
	}
	return NULL;
}

static void
activate(LV2_Handle instance)
{
	Matriseq* self = (Matriseq*)instance;
	self->naub = naub_world_new(self, pad_event);
	if (self->naub) {
		if (!naub_world_open(
			    self->naub, NAUB_VENDOR_NOVATION, NAUB_PRODUCT_LAUNCHPAD)) {
			if (zix_thread_create(&self->thread, 1024, pad_thread, self)) {
				print(self, self->uris.log_Error, "Failed to create thread\n");
				return;
			}
		} else {
			print(self, self->uris.log_Error, "Failed to open controller\n");
		}
	}
}

static void
run(LV2_Handle instance, uint32_t n_frames)
{
	Matriseq*           self = (Matriseq*)instance;
	const MatriseqURIs* uris = &self->uris;

	const float s_per_beat = 60.0f / self->bpm;
	const float s_per_step = s_per_beat * self->beats_per_bar / STEP_TYPE;

	// Prepare for writing to out port
	const uint32_t out_capacity = self->out->atom.size;
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->out, out_capacity);

	// Initialise output port to empty sequence
	LV2_Atom_Forge_Frame out_frame;
	lv2_atom_forge_sequence_head(&self->forge, &out_frame, 0);

	// Work forwards in time frame by frame, handling events as we go
	const LV2_Atom_Sequence* in = self->in;
	const LV2_Atom_Event*    ev = lv2_atom_sequence_begin(&in->body);
	for (uint32_t t = 0; t < n_frames; ++t) {
		while (!lv2_atom_sequence_is_end(&in->body, in->atom.size, ev) &&
		       ev->time.frames == t) {
			if (ev->body.type == uris->atom_Blank) {
				const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
				if (obj->body.otype == uris->time_Position) {
					// Update transport position and speed
					LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL;
					lv2_atom_object_get(obj,
					                    uris->time_barBeat, &beat,
					                    uris->time_beatsPerMinute, &bpm,
					                    uris->time_speed, &speed,
					                    NULL);
					if (bpm && bpm->type == uris->atom_Float) {
						self->bpm = ((LV2_Atom_Float*)bpm)->body;
					}
					if (beat && beat->type == uris->atom_Float) {
						self->time_frames = (((LV2_Atom_Float*)beat)->body
						                     * s_per_beat
						                     * self->rate);
					}
					if (speed && speed->type == uris->atom_Float) {
						self->speed = ((LV2_Atom_Float*)speed)->body;
					}
				}
			}

			ev = lv2_atom_sequence_next(ev);
		}

		const double   time_s = self->time_frames / self->rate;
		const uint32_t step   = (uint32_t)(time_s / s_per_step) % STEP_TYPE;
		if (step != self->step) {
			// Update step
			self->step = step;
			if (step == 0) {
				self->time_frames = 0;
			}

			// Notify USB thread of new step
			zix_ring_write(self->ring, &self->step, sizeof(self->step));

			// Send note ons for enabled notes this step
			for (uint32_t y = 0; y < SEQ_H; ++y) {
				if (self->seq[y][step]) {
					const uint8_t on[] = { 0x90, NOTE_MIN + y, 0x40 };
					lv2_atom_forge_frame_time(&self->forge, t);
					lv2_atom_forge_atom(&self->forge, 3, self->uris.midi_MidiEvent);
					lv2_atom_forge_write(&self->forge, on, 3);
				}
			}
		}

		if (self->speed) {
			++self->time_frames;
		}
	}
}

static void
deactivate(LV2_Handle instance)
{
	Matriseq* self = (Matriseq*)instance;
	self->exit = true;
	void* thread_ret = NULL;
	zix_thread_join(self->thread, &thread_ret);
	naub_world_free(self->naub);
	self->naub = NULL;
}

static void
cleanup(LV2_Handle instance)
{
	Matriseq* self = (Matriseq*)instance;
	zix_ring_free(self->ring);
	free(self);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	MATRISEQ_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
