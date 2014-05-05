/*
    adenv.c - A LV2 plugin to generate percussive (i.e no sustain time), linear AD envelopes.

    Copyright 2005 Loki Davison
    based on ADENV by Mike Rawes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define _XOPEN_SOURCE 500		/* strdup */
#include <stdlib.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <stdio.h>
#include <math.h>

#define ADENV_BASE_ID                   2661

#define ADENV_GATE                      0
#define ADENV_TRIGGER                   1
#define ADENV_ATTACK                    2
#define ADENV_DECAY                     3
#define ADENV_OUTPUT                    4

LV2_Descriptor **dahdsr_descriptors = 0;

typedef enum {
	IDLE,
	ATTACK,
	DECAY,
} ADENVState;

typedef struct {
	float *gate;
	float *trigger;
	float *attack;
	float *decay;
	float *output;
	float srate;
	float inv_srate;
	float last_gate;
	float last_trigger;
	float from_level;
	float level;
	ADENVState state;
	unsigned long samples;
} Dahdsr;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void* data)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	switch (port) {
	case ADENV_GATE:
		plugin->gate = data;
		break;
	case ADENV_TRIGGER:
		plugin->trigger = data;
		break;
	case ADENV_ATTACK:
		plugin->attack = data;
		break;
	case ADENV_DECAY:
		plugin->decay = data;
		break;
	case ADENV_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Dahdsr *plugin = (Dahdsr *) malloc(sizeof(Dahdsr));

	plugin->srate = (float) sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LV2_Handle) plugin;
}

static void
activate(LV2_Handle instance)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	plugin->last_gate = 0.0f;
	plugin->last_trigger = 0.0f;
	plugin->from_level = 0.0f;
	plugin->level = 0.0f;
	plugin->state = IDLE;
	plugin->samples = 0;
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	/* Gate */
	float *gate = plugin->gate;

	/* Trigger */
	float *trigger = plugin->trigger;

	/* Attack Time (s) */
	float attack = *(plugin->attack);

	/* Decay Time (s) */
	float decay = *(plugin->decay);

	/* Envelope Out */
	float *output = plugin->output;

	/* Instance Data */
	float srate = plugin->srate;
	float inv_srate = plugin->inv_srate;
	float last_gate = plugin->last_gate;
	float last_trigger = plugin->last_trigger;
	float from_level = plugin->from_level;
	float level = plugin->level;
	ADENVState state = plugin->state;
	unsigned long samples = plugin->samples;

	float gat, trg, att, dec;
	float elapsed;
	unsigned long s;

	/* Convert times into rates */
	att = attack > 0.0f ? inv_srate / attack : srate;
	dec = decay > 0.0f ? inv_srate / decay : srate;
	/* cuse's formula ...
	 * ReleaseCoeff = (ln(EndLevel) - ln(StartLevel)) / (EnvelopeDuration * SampleRate)
	 *
	 *  while (currentSample < endSample) Level += Level * ReleaseCoeff;
	 */

	float ReleaseCoeff = log(0.001) / (decay * srate);

	for (s = 0; s < sample_count; s++) {
		gat = gate[s];
		trg = trigger[s];

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f)) ||
			(gat > 0.0f && !(last_gate > 0.0f))) {
			//fprintf(stderr, "triggered in control \n");
			if (att <= srate) {
				state = ATTACK;
			}
			samples = 0;
		}

		if (samples == 0)
			from_level = level;

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			level = 0;
			break;
		case ATTACK:
			samples++;
			elapsed = (float) samples *att;

			if (elapsed > 1.0f) {
				state = DECAY;
				level = 1.0f;
				samples = 0;
			} else {
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case DECAY:
			samples++;
			elapsed = (float) samples *dec;

			if (elapsed > 1.0f) {
				state = IDLE;
				level = 0.0f;
				samples = 0;
			} else {
				//fprintf(stderr, "decay, dec %f elapsed %f from level %f level %f\n", dec, elapsed, from_level, level);
				level += level * ReleaseCoeff;

			}
			break;
		default:
			/* Should never happen */
			fprintf(stderr, "bugger!!!");
			level = 0.0f;
		}

		output[s] = level;
		last_gate = gat;
		last_trigger = trg;
	}

	plugin->last_gate = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->from_level = from_level;
	plugin->level = level;
	plugin->state = state;
	plugin->samples = samples;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/adenv",
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
