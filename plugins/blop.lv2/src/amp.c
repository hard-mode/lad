/*
  An LV2 plugin representing a simple mono amplifier.
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

#define AMP_GAIN   0
#define AMP_INPUT  1
#define AMP_OUTPUT 2

typedef struct {
	const float* gain;
	const float* input;
	float*       output;
	URIs         uris;
	uint32_t     gain_is_cv;
} Amp;

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
	Amp* plugin = (Amp*)instance;

	switch (port) {
	case AMP_GAIN:
		plugin->gain = (const float*)data;
		break;
	case AMP_INPUT:
		plugin->input = (const float*)data;
		break;
	case AMP_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Amp*     plugin = (Amp*)instance;
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
			case AMP_GAIN:
				plugin->gain_is_cv = (port_type == plugin->uris.lv2_CVPort);
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
	Amp* plugin = (Amp*)malloc(sizeof(Amp));
	if (!plugin) {
		return NULL;
	}

	plugin->gain_is_cv = 0;
	map_uris(&plugin->uris, features);

	return (LV2_Handle)plugin;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Amp* plugin = (Amp*)instance;

	/* Gain (dB) */
	const float* gain = plugin->gain;

	/* Input */
	const float* input = plugin->input;

	/* Output */
	float* output = plugin->output;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float gn    = gain[s * plugin->gain_is_cv];
		const float scale = (float)EXPF(M_LN10 * gn * 0.05f);

		output[s] = scale * input[s];
	}
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
	"http://drobilla.net/plugins/blop/amp",
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
