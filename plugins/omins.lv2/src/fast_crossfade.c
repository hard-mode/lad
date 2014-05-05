/* Crossfade with AR Level plugin.
 * Copyright 2005 Thorsten Wilms.
 *
 * Based on David Robillard's "Hz to AMS style V/Oct" plugin for the skeleton.
 * Thanks to Florian Schmidt for explaining how to calculate the scale values
 * before I could work it out myself! ;-)
 *
 * This plugin is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define XFADE_LEVEL_ID 4410

#define XFADE_NUM_PORTS 4

/* Port Numbers */
#define XFADE_LEVEL  0
#define XFADE_A      1
#define XFADE_B      2
#define XFADE_OUTPUT 3

/* All state information for plugin */
typedef struct {
	/* Ports */
	float *level_buffer;
	float *a_buffer;
	float *b_buffer;
	float *output_buffer;
} XFADE;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(XFADE));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
				   uint32_t port, void* location)
{
	XFADE* plugin;

	plugin = (XFADE*) instance;
	switch (port) {
	case XFADE_LEVEL:
		plugin->level_buffer = location;
		break;
	case XFADE_A:
		plugin->a_buffer = location;
		break;
	case XFADE_B:
		plugin->b_buffer = location;
		break;
	case XFADE_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	float*  level;
	float*  a;
	float*  b;
	float*  output;
	XFADE*        plugin;
	unsigned long i;
	float         l;

	plugin = (XFADE*)instance;

	level  = plugin->level_buffer;
	a      = plugin->a_buffer;
	b      = plugin->b_buffer;
	output = plugin->output_buffer;

	for (i = 0; i < nframes; i++) {
		/* transfer multiplication value to 0 to 1 range */
		if (level[i] < -1) {
			l = 0;
		} else if (level[i] > 1) {
			l = 1;
		} else {
			l = (level[i] + 1) / 2;
		}

		output[i] = a[i] * l + b[i] * (1 - l);
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/fast_crossfade",
	instantiate,
	connect_port,
	NULL,
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
