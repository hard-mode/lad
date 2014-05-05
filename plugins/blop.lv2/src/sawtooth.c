/*
  An LV2 plugin to generate a bandlimited sawtooth waveform.
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
#include "uris.h"
#include "wavedata.h"

#define SAWTOOTH_FREQUENCY 0
#define SAWTOOTH_OUTPUT    1

typedef struct {
	const float* frequency;
	float*       output;
	float        phase;
	uint32_t     frequency_is_cv;
	Wavedata     wdat;
	URIs         uris;
} Sawtooth;

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Sawtooth* plugin = (Sawtooth*)instance;

	switch (port) {
	case SAWTOOTH_FREQUENCY:
		plugin->frequency = (const float*)data;
		break;
	case SAWTOOTH_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Sawtooth* plugin = (Sawtooth*)instance;
	uint32_t  ret    = 0;
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
			case SAWTOOTH_FREQUENCY:
				plugin->frequency_is_cv = (port_type == plugin->uris.lv2_CVPort);
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
	Sawtooth* plugin = (Sawtooth*)malloc(sizeof(Sawtooth));
	if (!plugin) {
		return NULL;
	}

	if (wavedata_load(&plugin->wdat, bundle_path, "sawtooth_data",
	                  BLOP_DLSYM_SAWTOOTH, sample_rate)) {
		free(plugin);
		return 0;
	}

	plugin->frequency_is_cv = 0;
	map_uris(&plugin->uris, features);
	wavedata_get_table(&plugin->wdat, 440.0);

	return (LV2_Handle)plugin;
}

static void
cleanup(LV2_Handle instance)
{
	Sawtooth* plugin = (Sawtooth*)instance;

	wavedata_unload(&plugin->wdat);
	free(instance);
}

static void
activate(LV2_Handle instance)
{
	Sawtooth* plugin = (Sawtooth*)instance;

	plugin->phase = 0.0f;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sawtooth* plugin = (Sawtooth*)instance;

	/* Frequency (array of float of length 1 or sample_count) */
	const float* frequency = plugin->frequency;

	/* Output (pointer to float value) */
	float* output = plugin->output;

	/* Instance data */
	Wavedata* wdat  = &plugin->wdat;
	float     phase = plugin->phase;

	for (uint32_t s = 0; s < sample_count; s++) {
		const float freq = frequency[s * plugin->frequency_is_cv];
		if (freq != wdat->frequency) {
			/* Frequency changed, look up table to play */
			wavedata_get_table(wdat, freq);
		}

		output[s] = wavedata_get_sample(wdat, phase);

		/* Update phase, wrapping if necessary */
		phase += wdat->frequency;
		if (phase < 0.0f) {
			phase += wdat->sample_rate;
		} else if (phase > wdat->sample_rate) {
			phase -= wdat->sample_rate;
		}
	}
	plugin->phase = phase;
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
	"http://drobilla.net/plugins/blop/sawtooth",
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
