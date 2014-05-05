/* Hz to AMS style V/Oct plugin.  Copyright 2005-2011 David Robillard.
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

#define HZVOCT_BASE_ID 4200

#define HZVOCT_NUM_PORTS 2

/* Port Numbers */
#define HZVOCT_INPUT   0
#define HZVOCT_OUTPUT  1

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* input_buffer;
	float* output_buffer;
} HzVoct;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	HzVoct* plugin = malloc(sizeof(HzVoct));
	plugin->input_buffer = NULL;
	plugin->output_buffer = NULL;
	return (LV2_Handle)plugin;
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	HzVoct* plugin;

	plugin = (HzVoct*)instance;
	switch (port) {
	case HZVOCT_INPUT:
		plugin->input_buffer = location;
		break;
	case HZVOCT_OUTPUT:
		plugin->output_buffer = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	float*  input;
	float*  output;
	HzVoct*       plugin;
	unsigned long i;
	float         log2inv;
	float         eighth = 1.0f/8.0f;
	const float   offset = 5.0313842; // + octave, ... -1, 0, 1 ...

	plugin = (HzVoct*)instance;
	log2inv = 1.0f/logf(2.0);

	input = plugin->input_buffer;
	output = plugin->output_buffer;

	// Inverse of the formula used in AMS's converter module (except the 1/8 part)
	for (i = 0; i < nframes; i++)
		*output++ = logf((*input++) * eighth) * log2inv - offset;
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/hz_voct",
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
