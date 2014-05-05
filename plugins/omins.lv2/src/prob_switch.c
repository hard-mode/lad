/* This file is an audio plugin.
 * Copyright 2005 Loki Davison.
 *
 * Probability parameter is the prob of input 1 being the output value.
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

#define PROBSWITCH_BASE_ID 2667

#define PROBSWITCH_NUM_PORTS 4

/* Port Numbers */
#define PROBSWITCH_INPUT1   0
#define PROBSWITCH_INPUT2  1
#define PROBSWITCH_PROB  2
#define PROBSWITCH_OUTPUT  3

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* input2;
	float* prob;
	float* input1;
	float* output;
} ProbSwitch;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(ProbSwitch));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	ProbSwitch* plugin;

	plugin = (ProbSwitch*)instance;
	switch (port) {
	case PROBSWITCH_INPUT2:
		plugin->input2 = location;
		break;
	case PROBSWITCH_PROB:
		plugin->prob = location;
		break;
	case PROBSWITCH_INPUT1:
		plugin->input1 = location;
		break;
	case PROBSWITCH_OUTPUT:
		plugin->output = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	ProbSwitch*  plugin  = (ProbSwitch*)instance;
	float* input2  = plugin->input2;
	float* prob  = plugin->prob;
	float* input1   = plugin->input1;
	float* output  = plugin->output;
	size_t i;

	for (i = 0; i < nframes; ++i)
	{
    	    if((rand()/RAND_MAX) <= prob[i])
	    {
		output[i] = input1[i];
	    }
	     else
	    {
		output[i] = input2[i];
	    }
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/prob_switch",
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
