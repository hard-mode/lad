/*
  An LV2 plugin to split a signal into two.
  Copyright 2011-2014 David Robillard
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

#include <stdio.h>

#include <stdlib.h>
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "uris.h"

#define BRANCH_INPUT   0
#define BRANCH_OUTPUT1 1
#define BRANCH_OUTPUT2 2

typedef struct {
	const float* input;
	float*       output1;
	float*       output2;
	LV2_URID     input_type;
	URIs         uris;
} Branch;

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
	Branch* plugin = (Branch*)instance;

	switch (port) {
	case BRANCH_INPUT:
		plugin->input = (const float*)data;
		break;
	case BRANCH_OUTPUT1:
		plugin->output1 = (float*)data;
		break;
	case BRANCH_OUTPUT2:
		plugin->output2 = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Branch*  plugin = (Branch*)instance;
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
			if (port_type != plugin->uris.lv2_AudioPort &&
			    port_type != plugin->uris.lv2_CVPort &&
			    port_type != plugin->uris.lv2_ControlPort) {
				ret |= LV2_OPTIONS_ERR_BAD_VALUE;
				continue;
			}
			switch (o->subject) {
			case BRANCH_INPUT:
				plugin->input_type = port_type;
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
	const Branch* plugin = (const Branch*)instance;
	uint32_t      ret    = 0;
	for (LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT &&
		    o->subject != BRANCH_OUTPUT1 &&
		    o->subject != BRANCH_OUTPUT2) {
			fprintf(stderr, "Bad subject %d\n", o->subject);
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			fprintf(stderr, "Bad key %d\n", o->subject);
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else {
			o->size  = sizeof(LV2_URID);
			o->type  = plugin->uris.atom_URID;
			o->value = &plugin->input_type;
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
	Branch* plugin = (Branch*)malloc(sizeof(Branch));

	map_uris(&plugin->uris, features);
	plugin->input_type = plugin->uris.lv2_ControlPort;

	return (LV2_Handle)plugin;
}

static void
runBranch_ia_oaoa(LV2_Handle instance,
                  uint32_t   sample_count)
{
	Branch* plugin = (Branch*)instance;

	/* Input (array of floats of length sample_count) */
	const float* input = plugin->input;

	/* First Output (array of floats of length sample_count) */
	float* output1 = plugin->output1;

	/* Second Output (array of floats of length sample_count) */
	float* output2 = plugin->output2;

	float in;

	for (uint32_t s = 0; s < sample_count; ++s) {
		in = input[s];

		output1[s] = in;
		output2[s] = in;
	}
}

static void
runBranch_ic_ococ(LV2_Handle instance,
                  uint32_t   sample_count)
{
	Branch* plugin = (Branch*)instance;

	/* Input (float value) */
	const float input = *(plugin->input);

	/* First Output (pointer to float value) */
	float* output1 = plugin->output1;

	/* Second Output (pointer to float value) */
	float* output2 = plugin->output2;

	output1[0] = input;
	output2[0] = input;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Branch* plugin = (Branch*)instance;

	if (plugin->input_type == plugin->uris.lv2_AudioPort ||
	    plugin->input_type == plugin->uris.lv2_CVPort) {
		runBranch_ia_oaoa(instance, sample_count);
	} else {
		runBranch_ic_ococ(instance, sample_count);
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
	"http://drobilla.net/plugins/blop/branch",
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
