/*
  An LV2 plugin to shape a signal in various ways.
  Copyright 2011 David Robillard
  Copyright 2003 Mike Rawes

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
#include "common.h"
#include "uris.h"

#define TRACKER_GATE    0
#define TRACKER_HATTACK 1
#define TRACKER_HDECAY  2
#define TRACKER_LATTACK 3
#define TRACKER_LDECAY  4
#define TRACKER_INPUT   5
#define TRACKER_OUTPUT  6

typedef struct {
	const float* gate;
	const float* hattack;
	const float* hdecay;
	const float* lattack;
	const float* ldecay;
	const float* input;
	float*       output;
	float        coeff;
	float        last_value;
	uint32_t     hattack_is_cv;
	uint32_t     hdecay_is_cv;
	uint32_t     lattack_is_cv;
	uint32_t     ldecay_is_cv;
	URIs         uris;
} Tracker;

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
	Tracker* plugin = (Tracker*)instance;

	switch (port) {
	case TRACKER_GATE:
		plugin->gate = (const float*)data;
		break;
	case TRACKER_HATTACK:
		plugin->hattack = (const float*)data;
		break;
	case TRACKER_HDECAY:
		plugin->hdecay = (const float*)data;
		break;
	case TRACKER_LATTACK:
		plugin->lattack = (const float*)data;
		break;
	case TRACKER_LDECAY:
		plugin->ldecay = (const float*)data;
		break;
	case TRACKER_INPUT:
		plugin->input = (const float*)data;
		break;
	case TRACKER_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Tracker* plugin = (Tracker*)instance;
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
			case TRACKER_HATTACK:
				plugin->hattack_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case TRACKER_HDECAY:
				plugin->hdecay_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case TRACKER_LATTACK:
				plugin->lattack_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case TRACKER_LDECAY:
				plugin->ldecay_is_cv = (port_type == plugin->uris.lv2_CVPort);
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
	Tracker* plugin = (Tracker*)malloc(sizeof(Tracker));
	if (!plugin) {
		return NULL;
	}

	plugin->coeff = 2.0f * M_PI / (float)sample_rate;

	plugin->hattack_is_cv = 0;
	plugin->hdecay_is_cv  = 0;
	plugin->lattack_is_cv = 0;
	plugin->ldecay_is_cv  = 0;

	map_uris(&plugin->uris, features);

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Tracker* plugin = (Tracker*)instance;

	plugin->last_value = 0.0f;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Tracker* plugin = (Tracker*)instance;

	/* Gate (array of floats of length sample_count) */
	const float* gate = plugin->gate;

	/* Gate High Attack Rate (array of floats of length 1 or sample_count) */
	const float* hattack = plugin->hattack;

	/* Gate High Decay Rate (array of floats of length 1 or sample_count) */
	const float* hdecay = plugin->hdecay;

	/* Gate Low Attack Rate (array of floats of length 1 or sample_count) */
	const float* lattack = plugin->lattack;

	/* Gate Low Decay Rate (array of floats of length 1 or sample_count) */
	const float* ldecay = plugin->ldecay;

	/* Input (array of floats of length sample_count) */
	const float* input = plugin->input;

	/* Output (array of floats of length sample_count) */
	float* output = plugin->output;

	/* Instance Data */
	float coeff      = plugin->coeff;
	float last_value = plugin->last_value;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float in = input[s];
		const float ha = hattack[s * plugin->hattack_is_cv];
		const float hd = hdecay[s * plugin->hdecay_is_cv];
		const float la = lattack[s * plugin->lattack_is_cv];
		const float ld = ldecay[s * plugin->ldecay_is_cv];

		float rate;
		if (gate[s] > 0.0f) {
			rate = in > last_value ? ha : hd;
		} else {
			rate = in > last_value ? la : ld;
		}

		rate       = f_min(1.0f, rate * coeff);
		last_value = last_value * (1.0f - rate) + in * rate;

		output[s] = last_value;
	}

	plugin->last_value = last_value;
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
	"http://drobilla.net/plugins/blop/tracker",
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
