/*
  An LV2 plugin to modulate a frequency by a signal.
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
#include "math_func.h"
#include "uris.h"

#define FMOD_FREQUENCY 0
#define FMOD_MODULATOR 1
#define FMOD_OUTPUT    2

typedef struct {
	const float* frequency;
	const float* modulator;
	float*       output;
	uint32_t     frequency_is_cv;
	uint32_t     modulator_is_cv;
	uint32_t     output_is_cv;
	URIs         uris;
} Fmod;

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
	Fmod* plugin = (Fmod*)instance;

	switch (port) {
	case FMOD_FREQUENCY:
		plugin->frequency = (const float*)data;
		break;
	case FMOD_MODULATOR:
		plugin->modulator = (const float*)data;
		break;
	case FMOD_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Fmod*    plugin = (Fmod*)instance;
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
			case FMOD_FREQUENCY:
				plugin->frequency_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case FMOD_MODULATOR:
				plugin->modulator_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			default:
				ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
			}
		}
	}
	return ret;
}

static uint32_t
options_get(LV2_Handle          instance,
            LV2_Options_Option* options)
{
	const Fmod* plugin = (const Fmod*)instance;
	uint32_t    ret    = 0;
	for (LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT || o->subject != FMOD_OUTPUT) {
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else {
			o->size  = sizeof(LV2_URID);
			o->type  = plugin->uris.atom_URID;
			o->value = (plugin->output_is_cv
			            ? &plugin->uris.lv2_CVPort
			            : &plugin->uris.lv2_ControlPort);
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
	Fmod* plugin = (Fmod*)malloc(sizeof(Fmod));

	if (plugin) {
		plugin->frequency_is_cv = 0;
		plugin->modulator_is_cv = 0;
		plugin->output_is_cv    = 0;
		map_uris(&plugin->uris, features);
	}

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Fmod* plugin = (Fmod*)instance;

	/* Frequency to Modulate (array of floats of length 1 or sample_count) */
	const float* frequency = plugin->frequency;

	/* LFO Input (array of floats of length 1 or sample_count) */
	const float* modulator = plugin->modulator;

	/* Output Frequency (array of floats of length 1 or sample_count) */
	float* output = plugin->output;

	if (!plugin->output_is_cv) {  /* TODO: Avoid this branch */
		sample_count = 1;
	}

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float freq  = frequency[s * plugin->frequency_is_cv];
		const float mod   = modulator[s * plugin->modulator_is_cv];
		const float scale = (float)EXPF(M_LN2 * mod);

		output[s] = scale * freq;
	}
}

static const void*
extension_data(const char* uri)
{
	static const LV2_Options_Interface options = { options_get, options_set };
	if (!strcmp(uri, LV2_OPTIONS__interface)) {
		return &options;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/fmod",
	instantiate,
	connect_port,
	NULL,
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
