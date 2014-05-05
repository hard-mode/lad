/* Formant filter plugin.  Copyright 2005-2011 David Robillard.
 *
 * Based on SSM formant filter,
 * Copyright 2001 David Griffiths <dave@pawfal.org>
 *
 * Based on public domain code from alex@smartelectronix.com
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
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#define FORMANT_BASE_ID 4300

#define FORMANT_NUM_PORTS 3

/* Port Numbers */
#define FORMANT_VOWEL   0
#define FORMANT_INPUT   1
#define FORMANT_OUTPUT  2

/* Vowel Coefficients */
const double coeff[5][11] = {
	{ /* A */ 8.11044e-06,
		8.943665402, -36.83889529, 92.01697887, -154.337906, 181.6233289,
		-151.8651235, 89.09614114, -35.10298511, 8.388101016, -0.923313471
	},
	{ /* E */ 4.36215e-06,
	 8.90438318, -36.55179099, 91.05750846, -152.422234, 179.1170248,
		 -149.6496211, 87.78352223, -34.60687431, 8.282228154, -0.914150747
	},
	{ /* I */ 3.33819e-06,
	  8.893102966, -36.49532826, 90.96543286, -152.4545478, 179.4835618,
	  -150.315433, 88.43409371, -34.98612086, 8.407803364, -0.932568035
	},
	{ /* O */ 1.13572e-06,
	 8.994734087, -37.2084849, 93.22900521, -156.6929844, 184.596544,
	 -154.3755513, 90.49663749, -35.58964535, 8.478996281, -0.929252233
	},
	{ /* U */ 4.09431e-07,
	 8.997322763, -37.20218544, 93.11385476, -156.2530937, 183.7080141,
	 -153.2631681, 89.59539726, -35.12454591, 8.338655623, -0.910251753
	}
};

/* All state information for plugin */
typedef struct
{
	/* Ports */
	float* vowel;
	float* input;
	float* output;

	double memory[5][10];
} Formant;

/* Linear interpolation */
inline static float
linear(float bot, float top, float pos, float val1, float val2)
{
    float t = (pos - bot) / (top - bot);
    return val1 * t + val2 * (1.0f - t);
}

/* Construct a new plugin instance */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	return (LV2_Handle)malloc(sizeof(Formant));
}

/** Activate an instance */
static void
activate(LV2_Handle instance)
{
	Formant* plugin = (Formant*)instance;
	int i, j;

	for (i = 0; i < 5; ++i)
		for (j = 0; j < 10; ++j)
			plugin->memory[i][j] = 0.0;
}

/* Connect a port to a data location */
static void
connect_port(LV2_Handle instance,
                     uint32_t port,
                     void*  location)
{
	Formant* plugin;

	plugin = (Formant*)instance;
	switch (port) {
	case FORMANT_VOWEL:
		plugin->vowel = location;
		break;
	case FORMANT_INPUT:
		plugin->input = location;
		break;
	case FORMANT_OUTPUT:
		plugin->output = location;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t nframes)
{
	Formant*     plugin = (Formant*)instance;
	float  vowel;
	float  in;
	float* out;
	float  res;
	float  o[5];
	size_t       n, v;

	for (n=0; n < nframes; ++n) {
		vowel = plugin->vowel[0];
		in = plugin->input[n];
		out = plugin->output;

		for (v=0; v < 5; ++v) {
			res = (float) (coeff[v][0] * (in * 0.1f) +
			              coeff[v][1] * plugin->memory[v][0] +
			              coeff[v][2] * plugin->memory[v][1] +
			              coeff[v][3] * plugin->memory[v][2] +
			              coeff[v][4] * plugin->memory[v][3] +
			              coeff[v][5] * plugin->memory[v][4] +
			              coeff[v][6] * plugin->memory[v][5] +
			              coeff[v][7] * plugin->memory[v][6] +
			              coeff[v][8] * plugin->memory[v][7] +
			              coeff[v][9] * plugin->memory[v][8] +
			              coeff[v][10] * plugin->memory[v][9] );

			plugin->memory[v][9] = plugin->memory[v][8];
			plugin->memory[v][8] = plugin->memory[v][7];
			plugin->memory[v][7] = plugin->memory[v][6];
			plugin->memory[v][6] = plugin->memory[v][5];
			plugin->memory[v][5] = plugin->memory[v][4];
			plugin->memory[v][4] = plugin->memory[v][3];
			plugin->memory[v][3] = plugin->memory[v][2];
			plugin->memory[v][2] = plugin->memory[v][1];
			plugin->memory[v][1] = plugin->memory[v][0];
			plugin->memory[v][0] = (double)res;

			o[v] = res;
		}

		// Mix between formants
		if (vowel <= 0)
			out[n] = o[0];
		else if (vowel > 0 && vowel < 1)
			out[n] = linear(0.0f,  1.0f, vowel, o[1], o[0]);
		else if (vowel == 1)
			out[n] = o[1];
		else if (vowel > 1 && vowel < 2)
			out[n] = linear(0.0f,  1.0f, vowel - 1.0f, o[2], o[1]);
		else if (vowel == 2)
			out[n] = o[2];
		else if (vowel > 2 && vowel < 3)
			out[n] = linear(0.0f,  1.0f, vowel - 2.0f, o[3], o[2]);
		else if (vowel == 3)
			out[n] = o[3];
		else if (vowel > 3 && vowel < 4)
			out[n] = linear(0.0f,  1.0f, vowel - 3.0f, o[4], o[3]);
		else //if (vowel >= 4)
			out[n] = o[4];
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/formant_filter",
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
