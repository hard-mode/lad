/* slide.c - A LV2 plugin that "slides" the output signal between
 *           the current and the previous input value, taking the given
 *           number of seconds to reach the current input value.
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
#define SLIDE_INPUT                     0
#define SLIDE_RISETIME                  1
#define SLIDE_FALLTIME                  2
#define SLIDE_OUTPUT                    3

/* This is the data for a single instance of the plugin */
typedef struct
{
	float* input;
	float* risetime;
	float* falltime;
	float* output;
	float srate;
	float from;
	float to;
	float last_output;
} Slide;

/* Clean up after a plugin instance */
static void cleanup (LV2_Handle instance)
{
	free(instance);
}

/* This is called when the hosts connects a port to a buffer */
static void connect_port(LV2_Handle instance,
                      uint32_t port, void* data)
{
	Slide* plugin = (Slide *)instance;

	switch (port) {
	case SLIDE_INPUT:
		plugin->input = data;
		break;
	case SLIDE_RISETIME:
		plugin->risetime = data;
		break;
	case SLIDE_FALLTIME:
		plugin->falltime = data;
		break;
	case SLIDE_OUTPUT:
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
	Slide* plugin = (Slide*)calloc(1, sizeof(Slide));
	plugin->srate = (float)sample_rate;
	return (LV2_Handle)plugin;
}

/* This is called before the hosts starts to use the plugin instance */
static void activate(LV2_Handle instance)
{
	Slide* plugin = (Slide*)instance;
	plugin->last_output = 0;
	plugin->from = 0;
	plugin->to = 0;
}

/* The run function! */
static void run(LV2_Handle instance, uint32_t sample_count)
{
	Slide* plugin = (Slide*)instance;

	if (plugin->input && plugin->output) {
		unsigned long i;
		float risetime, falltime;
		for (i = 0; i < sample_count; ++i) {

			if (plugin->risetime)
				risetime = plugin->risetime[i];
			else
				risetime = 0;

			if (plugin->falltime)
				falltime = plugin->falltime[i];
			else
				falltime = 0;

			/* If the signal changed, start sliding to the new value */
			if (plugin->input[i] != plugin->to) {
				plugin->from = plugin->last_output;
				plugin->to = plugin->input[i];
			}

			float time;
			int rising;
			if (plugin->to > plugin->from) {
				time = risetime;
				rising = 1;
			} else {
				time = falltime;
				rising = 0;
			}

			/* If the rise/fall time is 0, just copy the input to the output */
			if (time == 0)
				plugin->output[i] = plugin->input[i];

			/* Otherwise, do the portamento */
			else {
				float increment =
				    (plugin->to - plugin->from) / (time * plugin->srate);
				plugin->output[i] = plugin->last_output + increment;
				if ((plugin->output[i] > plugin->to && rising) ||
				        (plugin->output[i] < plugin->to && !rising)) {
					plugin->output[i] = plugin->to;
				}
			}

			plugin->last_output = plugin->output[i];
		}
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/slide",
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
