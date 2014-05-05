/*
  An LV2 plugin to generate a random wave of varying frequency and smoothness.
  Copyright 2011 David Robillard
  Copyright 2002 Mike Rawes

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <time.h>
#include "math_func.h"
#include "common.h"
#include "uris.h"

#define RANDOM_FREQUENCY 0
#define RANDOM_SMOOTH    1
#define RANDOM_OUTPUT    2

typedef struct {
	const float* frequency;
	const float* smooth;
	float*       output;
	float        nyquist;
	float        inv_nyquist;
	float        phase;
	float        value1;
	float        value2;
	uint32_t     frequency_is_cv;
	uint32_t     smooth_is_cv;
	URIs         uris;
} Random;

float inv_rand_max;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Random* plugin = (Random*)instance;

	switch (port) {
	case RANDOM_FREQUENCY:
		plugin->frequency = (const float*)data;
		break;
	case RANDOM_SMOOTH:
		plugin->smooth = (const float*)data;
		break;
	case RANDOM_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Random*  plugin = (Random*)instance;
	uint32_t ret    = 0;
	for (const LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT) {
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else if (o->type != plugin->uris.atom_URID) {
			ret |= LV2_OPTIONS_ERR_BAD_VALUE;
		} else {
			LV2_URID port_type = *(const LV2_URID*)(o->value);
			if (port_type != plugin->uris.lv2_ControlPort &&
			    port_type != plugin->uris.lv2_CVPort) {
				ret |= LV2_OPTIONS_ERR_BAD_VALUE;
				continue;
			}

			switch (o->subject) {
			case RANDOM_FREQUENCY:
				plugin->frequency_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case RANDOM_SMOOTH:
				plugin->smooth_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			default:
				ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
			}
		}
	}
	return ret;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Random* plugin = (Random*)malloc(sizeof(Random));
	if (!plugin) {
		return NULL;
	}

	srand((int)time((time_t*)0));

	inv_rand_max = 2.0f / (float)RAND_MAX;

	plugin->nyquist     = (float)sample_rate / 2.0f;
	plugin->inv_nyquist = 1.0f / plugin->nyquist;

	plugin->value1 = rand() * inv_rand_max - 1.0f;
	plugin->value2 = rand() * inv_rand_max - 1.0f;

	plugin->frequency_is_cv = 0;
	plugin->smooth_is_cv = 0;

	map_uris(&plugin->uris, features);

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Random* plugin = (Random*)instance;

	plugin->phase = 0.0f;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Random* plugin = (Random*)instance;

	/* Frequency (Hz) (array of floats of length 1 or sample_count) */
	const float* frequency = plugin->frequency;

	/* Wave smoothness (array of floats of length 1 or sample_count) */
	const float* smooth = plugin->smooth;

	/* Output (array of floats of length sample_count) */
	float* output = plugin->output;

	/* Instance data */
	float nyquist     = plugin->nyquist;
	float inv_nyquist = plugin->inv_nyquist;
	float phase       = plugin->phase;
	float value1      = plugin->value1;
	float value2      = plugin->value2;

	float result;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float freq = f_clip(frequency[s * plugin->frequency_is_cv],
		                          0.0f, nyquist);

		const float smth = f_clip(smooth[s * plugin->smooth_is_cv],
		                          0.0f, 1.0f);

		const float interval = (1.0f - smth) * 0.5f;

		if (phase < interval) {
			result = 1.0f;
		} else if (phase > (1.0f - interval)) {
			result = -1.0f;
		} else if (interval > 0.0f) {
			result = COSF((phase - interval) / smth * M_PI);
		} else {
			result = COSF(phase * M_PI);
		}

		result *= (value2 - value1) * 0.5f;
		result -= (value2 + value1) * 0.5f;

		output[s] = result;

		phase += freq * inv_nyquist;
		if (phase > 1.0f) {
			phase -= 1.0f;
			value1 = value2;
			value2 = (float)rand() * inv_rand_max - 1.0f;
		}
	}

	plugin->phase  = phase;
	plugin->value1 = value1;
	plugin->value2 = value2;
}

static const void*
extension_data(const char* uri)
{
	static const LV2_Options_Interface options = { NULL, options_set };
	if (!strcmp(uri, LV2_OPTIONS__interface)) {
		return &options;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/random",
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	extension_data,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
