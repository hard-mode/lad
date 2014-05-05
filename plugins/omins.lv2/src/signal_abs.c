/* Absolute value audio plugin.
 * Copyright 2005 Loki Davison.
 *
 * Sign parameter is the sign of the output, 0 being negative and >0 begin positive.
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

#define SIGNAL_ABS_BASE_ID 2669

#define SIGNAL_ABS_NUM_PORTS 3

/* Port Numbers */
#define SIGNAL_ABS_INPUT1   0
#define SIGNAL_ABS_SIGN  1
#define SIGNAL_ABS_OUTPUT  2

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* sign;
	float* input1;
	float* output;
} SignalAbs;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(SignalAbs));
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	SignalAbs* plugin;

	plugin = (SignalAbs*)instance;
	switch (port) {
	case SIGNAL_ABS_SIGN:
		plugin->sign = location;
		break;
	case SIGNAL_ABS_INPUT1:
		plugin->input1 = location;
		break;
	case SIGNAL_ABS_OUTPUT:
		plugin->output = location;
		break;
	}
}

static void
run_ar(LV2_Handle instance, uint32_t nframes)
{
	SignalAbs*  plugin  = (SignalAbs*)instance;
	float* sign  = plugin->sign;
	float* input1   = plugin->input1;
	float* output  = plugin->output;
	size_t i;

	for (i = 0; i < nframes; ++i)
	{
    	    if(sign[i] > 0.5)
	    {
		output[i] = fabs(input1[i]);
	    }
	     else
	    {
		output[i] = fabs(input1[i]) * -1;
	    }
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/signal_abs",
	instantiate,
	connect_port,
	NULL,
	run_ar,
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
