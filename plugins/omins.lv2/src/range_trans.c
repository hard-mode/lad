/* This file is an audio plugin.  Copyright 2005-2011 David Robillard.
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

#define RANGETRANS_BASE_ID 4210

#define RANGETRANS_NUM_PORTS 6

/* Port Numbers */
#define RANGETRANS_IN_MIN  0
#define RANGETRANS_IN_MAX  1
#define RANGETRANS_OUT_MIN 2
#define RANGETRANS_OUT_MAX 3
#define RANGETRANS_INPUT   4
#define RANGETRANS_OUTPUT  5

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* in_min;
	float* in_max;
	float* out_min;
	float* out_max;
	float* input;
	float* output;
} RangeTrans;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(RangeTrans));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	RangeTrans* plugin;

	plugin = (RangeTrans*)instance;
	switch (port) {
	case RANGETRANS_IN_MIN:
		plugin->in_min = location;
		break;
	case RANGETRANS_IN_MAX:
		plugin->in_max = location;
		break;
	case RANGETRANS_OUT_MIN:
		plugin->out_min = location;
		break;
	case RANGETRANS_OUT_MAX:
		plugin->out_max = location;
		break;
	case RANGETRANS_INPUT:
		plugin->input = location;
		break;
	case RANGETRANS_OUTPUT:
		plugin->output = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	const RangeTrans* const  plugin  = (RangeTrans*)instance;
	const float* const in_min  = plugin->in_min;
	const float* const in_max  = plugin->in_max;
	const float* const out_min = plugin->out_min;
	const float* const out_max = plugin->out_max;
	const float* const input   = plugin->input;
	float* const       output  = plugin->output;
	unsigned long i;

	for (i = 0; i < nframes; ++i)
		output[i] = ((input[i] - in_min[i]) / (in_max[i] - in_min[i]))
			* (out_max[i] - out_min[i]) + out_min[i];
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/range_trans",
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
