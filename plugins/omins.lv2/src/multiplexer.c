/* Multiplxer plugin.
 * Copyright 2005 Thorsten Wilms.
 * Based on David Robillard's "Hz to AMS style V/Oct" plugin for the skeleton.
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
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define MUX_GATE_ID 4420

#define MUX_NUM_PORTS 4

/* Port Numbers */
#define MUX_GATE     0
#define MUX_OFF     1
#define MUX_ON  2
#define MUX_OUTPUT   3

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* gate_buffer;
	float* off_buffer;
	float* on_buffer;
        float* output_buffer;
} MUX;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(MUX));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	MUX* plugin;

	plugin = (MUX*)instance;
	switch (port) {
	case MUX_GATE:
		plugin->gate_buffer = location;
		break;
        case MUX_OFF:
		plugin->off_buffer = location;
		break;
        case MUX_ON:
		plugin->on_buffer = location;
		break;
	case MUX_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	const MUX* const         plugin = (MUX*)instance;
	const float* const gate   = plugin->gate_buffer;
	const float* const off    = plugin->off_buffer;
	const float* const on     = plugin->on_buffer;
	float* const       output = plugin->output_buffer;
	unsigned long i;

	for (i = 0; i < nframes; i++)
		output[i] = (gate[i] <= 0) ? off[i] : on[i];
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/multiplexer",
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
