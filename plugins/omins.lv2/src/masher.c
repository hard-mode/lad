/*  Masher
 *  Copyright 2001 David Griffiths <dave@pawfal.org>
 *  LV2fication 2005 David Robillard <drobilla@connect.carelton.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* NOTE:  This is a very dirty hack full of arbitrary limits and assumptions.
 * It needs fixing/completion */

#define _XOPEN_SOURCE 600 /* posix_memalign */
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define MASHER_BASE_ID 4310

#define MASHER_NUM_PORTS 4

/* Port Numbers */
#define MASHER_INPUT      0
#define MASHER_GRAINPITCH 1
#define MASHER_DENSITY    2
#define MASHER_OUTPUT     3

#define GRAINSTORE_SIZE 1000
#define OVERLAPS_SIZE 1000
#define MAX_GRAIN_SIZE 2048

typedef struct {
	float* data;
	size_t       length;
} Sample;

typedef struct {
	int pos;
	int grain;
} GrainDesc;

/* All state information for plugin */
typedef struct {
	/* Ports */
	float *input;
	float *grain_pitch;
	float *density;
	float *output;

	Sample grain_store[GRAINSTORE_SIZE];
	GrainDesc overlaps[OVERLAPS_SIZE];
	size_t overlaps_size;

	size_t write_grain;
} Masher;

static inline float
rand_range(float l, float h)
{
	return ((rand() % 10000 / 10000.0f) * (h - l)) + l;
}

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(Masher));
}

/** Activate an instance */
static void
activate(LV2_Handle instance)
{
	Masher *plugin = (Masher*)instance;
	int i = 0;

	plugin->overlaps_size = 0;
	plugin->write_grain = 0;

	for (i=0; i < GRAINSTORE_SIZE; ++i) {
		//plugin->grain_store[i].data = (float*)calloc(MAX_GRAIN_SIZE, sizeof(float));
		posix_memalign((void**)&plugin->grain_store[i].data, 16, MAX_GRAIN_SIZE * sizeof(float));
		plugin->grain_store[i].length = 0;
	}
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port, void * location)
{
	Masher *plugin = (Masher *) instance;

	switch (port) {
	case MASHER_INPUT:
		plugin->input = location;
		break;
	case MASHER_GRAINPITCH:
		plugin->grain_pitch = location;
		break;
	case MASHER_DENSITY:
		plugin->density = location;
		break;
	case MASHER_OUTPUT:
		plugin->output = location;
		break;
	}
}

static void
mix_pitch(Sample* src, Sample* dst, size_t pos, float pitch)
{
	float  n = 0;
	size_t p = pos;

	while (n < src->length && p < dst->length) {
		dst->data[p] = dst->data[p] + src->data[(size_t)n];
		n += pitch;
		p++;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	Masher* plugin = (Masher*)instance;

	static const int randomness       = 1.0; // FIXME: make a control port
	int              read_grain       = 0;   // FIXME: what is this?
	int              grain_store_size = 100; // FIXME: what is this? (max 1000)

	const float grain_pitch = *plugin->grain_pitch;
	const float density     = *plugin->density;

	const float* const in  = plugin->input;
	float* const       out = plugin->output;

	Sample out_sample = { out, nframes };

	size_t n           = 0;
	float  s           = in[0];
	int    last        = 0;
	bool   first       = true;
	size_t grain_index = 0;
	size_t next_grain  = 0;

	// Zero output buffer
	for (n = 0; n < nframes; ++n)
		out[n] = 0.0f;

	// Paste any overlapping grains to the start of the buffer.
	for (n = 0; n < plugin->overlaps_size; ++n) {
		mix_pitch(&plugin->grain_store[plugin->overlaps[n].grain], &out_sample,
				plugin->overlaps[n].pos - nframes, grain_pitch);
	}
	plugin->overlaps_size = 0;

	// Chop up the buffer and put the grains in the grainstore
	for (n = 0; n < nframes; n++) {
		if ((s < 0 && in[n] > 0) || (s > 0 && in[n] < 0)) {
			// Chop the bits between zero crossings
			if (!first) {
				if (n - last <= MAX_GRAIN_SIZE) {
					grain_index = plugin->write_grain % grain_store_size;
					memcpy(plugin->grain_store[grain_index].data, in, nframes);
					plugin->grain_store[grain_index].length = n - last;
				}
				plugin->write_grain++; // FIXME: overflow?
			} else {
				first = false;
			}

			last = n;
			s = in[n];
		}
	}

	for (n = 0; n < nframes; n++) {
		if (n >= next_grain || rand() % 1000 < density) {
			size_t grain_num = read_grain % grain_store_size;
			mix_pitch(&plugin->grain_store[grain_num], &out_sample, n, grain_pitch);
			size_t grain_length = (plugin->grain_store[grain_num].length * grain_pitch);

			next_grain = n + plugin->grain_store[grain_num].length;

			// If this grain overlaps the buffer
			if (n + grain_length > nframes) {
				if (plugin->overlaps_size < OVERLAPS_SIZE) {
					GrainDesc new_grain;

					new_grain.pos = n;
					new_grain.grain = grain_num;
					plugin->overlaps[plugin->overlaps_size++] = new_grain;
				}
			}

			if (randomness)
				read_grain += 1 + rand() % randomness;
			else
				read_grain++;
		}
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}


static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/masher",
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	NULL,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
