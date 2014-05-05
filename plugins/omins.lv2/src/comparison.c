/* Comparison plugin.
 * Copyright 2005 Thorsten Wilms.
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

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#include "lv2.h"

#define COMP_BASE_ID 4440

#define COMP_NUM_PORTS 6

/* Port Numbers */
#define COMP_A        0
#define COMP_B        1
#define COMP_LARGER   2
#define COMP_SMALLER  3
#define COMP_A_LARGER 4
#define COMP_EQUAL    5

/* All state information for plugin */
typedef struct {
	/* Ports */
	float *a_buffer;
	float *b_buffer;
	float *larger_buffer;
	float *smaller_buffer;
	float *a_larger_buffer;
	float *equal_buffer;
} Comp;

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Comp* plugin = malloc(sizeof(Comp));
	plugin->a_buffer        = NULL;
	plugin->b_buffer        = NULL;
	plugin->larger_buffer   = NULL;
	plugin->smaller_buffer  = NULL;
	plugin->a_larger_buffer = NULL;
	plugin->equal_buffer    = NULL;
	return (LV2_Handle)plugin;
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void*  location)
{
	Comp* plugin;

	plugin = (Comp*)instance;
	switch (port) {
	case COMP_A:
		plugin->a_buffer = location;
		break;
	case COMP_B:
		plugin->b_buffer = location;
		break;
	case COMP_LARGER:
		plugin->larger_buffer = location;
		break;
	case COMP_SMALLER:
		plugin->smaller_buffer = location;
		break;
	case COMP_A_LARGER:
		plugin->a_larger_buffer = location;
		break;
	case COMP_EQUAL:
		plugin->equal_buffer = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	Comp* const              plugin   = (Comp*)instance;
	const float* const a        = plugin->a_buffer;
	const float* const b        = plugin->b_buffer;
	float* const       larger   = plugin->larger_buffer;
	float* const       smaller  = plugin->smaller_buffer;
	float* const       a_larger = plugin->a_larger_buffer;
	float* const       equal    = plugin->equal_buffer;
	unsigned long i;

	for (i = 0; i < nframes; i++) {
		equal[i]    = (a[i] == b[i]) ? 1.0  : 0.0;
		larger[i]   = (a[i] > b[i])  ? a[i] : b[i];
		smaller[i]  = (a[i] < b[i])  ? a[i] : b[i];
		a_larger[i] = (a[i] > b[i])  ? 1.0  : 0.0;
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/comparison",
	instantiate,
	connect_port,
	NULL,
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
