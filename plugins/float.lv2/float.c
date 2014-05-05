/* Float.lv2
 * Copyright 2011-2011 David Robillard <http://drobilla.net/>
 *
 * Float.lv2 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Float.lv2 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define FLOAT_URI "http://drobilla.net/plugins/float"
#define FLOAT_IN  0
#define FLOAT_OUT 1

static LV2_Descriptor* plugin_descriptor = NULL;

typedef struct {
	float* in;
	float* out;
} FloatPlugin;

static void
float_cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
float_connect_port(LV2_Handle instance,
                   uint32_t   port,
                   void*      data)
{
	FloatPlugin* me = (FloatPlugin*)instance;

	switch (port) {
	case FLOAT_IN:
		me->in = data;
		break;
	case FLOAT_OUT:
		me->out = data;
		break;
	}
}

static LV2_Handle
float_instantiate(const LV2_Descriptor*     descriptor,
                  double                    rate,
                  const char*               path,
                  const LV2_Feature* const* features)
{
	FloatPlugin* me = (FloatPlugin*)malloc(sizeof(FloatPlugin));

	return (LV2_Handle)me;
}

static void
float_run(LV2_Handle instance,
          uint32_t   sample_count)
{
	FloatPlugin* me = (FloatPlugin*)instance;

	*me->out = *me->in;
}

static void
init(void)
{
	plugin_descriptor = (LV2_Descriptor*)malloc(sizeof(LV2_Descriptor));
	plugin_descriptor->URI            = FLOAT_URI;
	plugin_descriptor->activate       = NULL;
	plugin_descriptor->cleanup        = float_cleanup;
	plugin_descriptor->connect_port   = float_connect_port;
	plugin_descriptor->deactivate     = NULL;
	plugin_descriptor->instantiate    = float_instantiate;
	plugin_descriptor->run            = float_run;
	plugin_descriptor->extension_data = NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!plugin_descriptor) {
		init();
	}

	switch (index) {
	case 0:
		return plugin_descriptor;
	default:
		return NULL;
	}
}
