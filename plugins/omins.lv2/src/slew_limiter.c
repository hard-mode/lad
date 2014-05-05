/* slew_limiter - A LV2 plugin that limits the rate of change of a
 *                signal. Increases and decreases in the signal can be
 *                limited separately.
 *
 * Copyright 2005 Lars Luthman
 * LV2 skeleton code taken from dahdsr_fexp.c by Loki Davison
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
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <math.h>

/* These are the port numbers */
#define SLIM_INPUT                     0
#define SLIM_MAXRISE                   1
#define SLIM_MAXFALL                   2
#define SLIM_OUTPUT                    3

/* This is an array pointer to the descriptors of the different variants */
LV2_Descriptor** slim_descriptors = 0;

/* This is the data for a single instance of the plugin */
typedef struct
{
	float* input;
	float* maxrise;
	float* maxfall;
	float* reset;
	float* output;
	float srate;
	float last_output;
}
SLim;

/* Clean up after a plugin instance */
static void cleanup (LV2_Handle instance)
{
	free(instance);
}

/* This is called when the hosts connects a port to a buffer */
static void connect_port(LV2_Handle instance,
                     uint32_t port, void* data)
{
	SLim* plugin = (SLim *)instance;

	switch (port) {
	case SLIM_INPUT:
		plugin->input = data;
		break;
	case SLIM_MAXRISE:
		plugin->maxrise = data;
		break;
	case SLIM_MAXFALL:
		plugin->maxfall = data;
		break;
	case SLIM_OUTPUT:
		plugin->output = data;
		break;
	}
}

/* The host uses this function to create an instance of a plugin */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	SLim* plugin = (SLim*)calloc(1, sizeof(SLim));
	plugin->srate = (float)sample_rate;
	return (LV2_Handle)plugin;
}

/* This is called before the hosts starts to use the plugin instance */
static void activate(LV2_Handle instance)
{
	SLim* plugin = (SLim*)instance;
	plugin->last_output = 0;
}

/* The run function! */
static void run(LV2_Handle instance, uint32_t sample_count)
{
	SLim* plugin = (SLim*)instance;

	if (plugin->input && plugin->output) {
		unsigned long i;
		float maxrise, maxfall;
		for (i = 0; i < sample_count; ++i) {

			if (plugin->maxrise)
				maxrise = plugin->maxrise[i];
			else
				maxrise = 0;

			if (plugin->maxfall)
				maxfall = plugin->maxfall[i];
			else
				maxfall = 0;

			float maxinc = maxrise / plugin->srate;
			float maxdec = maxfall / plugin->srate;
			float increment = plugin->input[i] - plugin->last_output;
			if (increment > maxinc)
				increment = maxinc;
			else if (increment < -maxdec)
				increment = -maxdec;

			plugin->output[i] = plugin->last_output + increment;
			plugin->last_output = plugin->output[i];
		}
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/slew_limiter",
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
