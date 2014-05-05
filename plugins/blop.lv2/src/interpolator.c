/*
  An LV2 plugin to generate a smooth audio signal from a control source.
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
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define INTERPOLATOR_INPUT  0
#define INTERPOLATOR_OUTPUT 1

/**
   Mutated spline interpolator using only two previous samples and one next.
   @param interval Normalised time interval between inteprolated sample and p0
   @param p1 Sample two previous to interpolated one
   @param p0 Previous sample to interpolated one
   @param n0 Sample following interpolated one
   @return Interpolated sample.
*/
static inline float
interpolate(float interval,
            float p1,
            float p0,
            float n0)
{
	return p0 + 0.5f * interval * (n0 - p1 +
	                   interval * (4.0f * n0 + 2.0f * p1 - 5.0f * p0 - n0 +
	                   interval * (3.0f * (p0 - n0) - p1 + n0)));
}

typedef struct {
	const float* input;
	float*       output;
	float        p1;
	float        p0;
} Interpolator;

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
	Interpolator* plugin = (Interpolator*)instance;

	switch (port) {
	case INTERPOLATOR_INPUT:
		plugin->input = (const float*)data;
		break;
	case INTERPOLATOR_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Interpolator* plugin = (Interpolator*)malloc(sizeof(Interpolator));
	if (!plugin) {
		return NULL;
	}

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Interpolator* plugin = (Interpolator*)instance;

	plugin->p1 = 0.0f;
	plugin->p0 = 0.0f;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Interpolator* plugin = (Interpolator*)instance;

	/* Control Input (float value) */
	const float input = *(plugin->input);

	/* Interpolated Output (pointer to float value) */
	float* output = plugin->output;

	/* We use two previous values and the input as the 'next' one */
	float p1 = plugin->p1;
	float p0 = plugin->p0;

	const float inv_scount = 1.0f / (float)sample_count;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float interval = (float)s * inv_scount;

		output[s] = interpolate(interval, p1, p0, input);
	}

	plugin->p1 = p0;
	plugin->p0 = input;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/interpolator",
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
