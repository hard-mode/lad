/* Sample and Hold.
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

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define ID 4430

#define NUM_PORTS 5

/* Port Numbers */
#define INPUT      0
#define TRIGGER    1
#define THRESHOLD  2
#define CONTINUOUS 3
#define OUTPUT     4

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* input_buffer;
	float* trigger_buffer;
	float* threshold_buffer;
	float* continuous_buffer;
	float* output_buffer;

	float hold; /* the value sampled and held */
	float last_trigger; /* trigger port value from the previous frame */
} SH;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(SH));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	SH* plugin = (SH*)instance;

	switch (port) {
	case INPUT:
		plugin->input_buffer = location;
		break;
	case TRIGGER:
		plugin->trigger_buffer = location;
		break;
	case THRESHOLD:
		plugin->threshold_buffer = location;
		break;
	case CONTINUOUS:
		plugin->continuous_buffer = location;
		break;
	case OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}

static void
activate(LV2_Handle instance)
{
	SH* plugin = (SH*)instance;

	plugin->hold = 0;
	plugin->last_trigger = 0;
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	SH* const                plugin       = (SH*)instance;
	const float* const input        = plugin->input_buffer;
	const float* const trigger      = plugin->trigger_buffer;
	const float* const threshold    = plugin->threshold_buffer;
	const float* const continuous   = plugin->continuous_buffer;
	float* const       output       = plugin->output_buffer;

	unsigned long i;

	for (i = 0; i < nframes; i++) {
		if (continuous[0] != 0) {
			/* Continuous triggering on (sample while trigger > threshold) */
			if (trigger[i] >= threshold[i])
				plugin->hold = input[i];
		} else {
			/* Continuous triggering off
			 * (only sample on first frame with trigger > threshold) */
			if (plugin->last_trigger < threshold[i] && trigger[i] >= threshold[i])
				plugin->hold = input[i];
		}

		plugin->last_trigger = trigger[i];
		output[i] = plugin->hold;
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/sample_and_hold",
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
