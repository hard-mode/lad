/*
    adenv.c - A LV2 plugin to generate percussive (i.e no sustain time), linear AD envelopes.
    This one takes in levels to make  filter sweeps/etc easier.

    Copyright 2005 Loki Davison
    based of DADSR by Mike Rawes

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

#define ADENVLVL_BASE_ID                   2662
#define ADENVLVL_VARIANT_COUNT             1

#define ADENVLVL_GATE                      0
#define ADENVLVL_TRIGGER                   1
#define ADENVLVL_START_LEVEL		 2
#define ADENVLVL_ATTACK_LEVEL		 3
#define ADENVLVL_DECAY_LEVEL               4
#define ADENVLVL_ATTACK                    5
#define ADENVLVL_DECAY                     6
#define ADENVLVL_OUTPUT                    7
#define ADENVLVL_RESET                  	 8

typedef enum {
	IDLE,
	ATTACK,
	DECAY,
} ADENVLVLState;

typedef struct {
	float *gate;
	float *trigger;
	float *attack;
	float *reset;
	float *decay;
	float *start_level;
	float *attack_level;
	float *decay_level;
	float *output;
	float srate;
	float inv_srate;
	float last_gate;
	float last_trigger;
	float last_reset;
	float level;
	ADENVLVLState state;
	unsigned long samples;
} Dahdsr;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t port, void * data)
{
	Dahdsr *plugin = (Dahdsr *) instance;

	switch (port) {
	case ADENVLVL_GATE:
		plugin->gate = data;
		break;
	case ADENVLVL_TRIGGER:
		plugin->trigger = data;
		break;
	case ADENVLVL_START_LEVEL:
		plugin->start_level = data;
		break;
	case ADENVLVL_ATTACK_LEVEL:
		plugin->attack_level = data;
		break;
	case ADENVLVL_DECAY_LEVEL:
		plugin->decay_level = data;
		break;
	case ADENVLVL_ATTACK:
		plugin->attack = data;
		break;
	case ADENVLVL_DECAY:
		plugin->decay = data;
		break;
	case ADENVLVL_RESET:
		plugin->reset = data;
		break;
	case ADENVLVL_OUTPUT:
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
	plugin->last_reset = 0.0f;
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

	/* Reset */
	float *reset = plugin->reset;

	/* Start Level */
	float start_level = *(plugin->start_level);

	/* Attack Level */
	float attack_level = *(plugin->attack_level);

	/* Decay Level */
	float decay_level = *(plugin->decay_level);

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
	float last_reset = plugin->last_reset;
	float level = plugin->level;
	ADENVLVLState state = plugin->state;
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
	/* check params don't cause div by zero */
	if (start_level == 0) {
		start_level = 0.0000001;
	}
	if (attack_level == 0) {
		attack_level = 0.0000001;
	}
	if (decay_level == 0) {
		decay_level = 0.0000001;
	}
	float ReleaseCoeff_att =
		(log(attack_level) - log(start_level)) / (attack * srate);
	float ReleaseCoeff_dec =
		(log(decay_level) - log(attack_level)) / (decay * srate);

	for (s = 0; s < sample_count; s++) {
		gat = gate[s];
		trg = trigger[s];

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f)) ||
			(gat > 0.0f && !(last_gate > 0.0f))) {
			//fprintf(stderr, "triggered in control \n");
			if (att < srate) {
				state = ATTACK;
			}
			samples = 0;
		}
		/* if we got a reset */

		if (reset[s] > 0.0f && !(last_reset > 0.0f)) {
			level = start_level;
			/*fprintf(stderr, "got reset start level %f \n", start_level); */
		}

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			/* might need to fix this... */
			break;
		case ATTACK:
			/* fix level adding prob */
			if (samples == 0) {
				level = start_level;
			}
			samples++;
			elapsed = (float) samples *att;

			if (elapsed > 1.0f) {
				state = DECAY;
				samples = 0;
				//fprintf(stderr, "finished attack, RC %f, level %f attack_level %f start %f\n", ReleaseCoeff_att, level, attack_level, start_level);
			} else {
				level += level * ReleaseCoeff_att;
			}
			break;
		case DECAY:
			samples++;
			elapsed = (float) samples *dec;

			if (elapsed > 1.0f) {
				//fprintf(stderr, "finished decay, RC %f , level %f decay_level %f start %f\n", ReleaseCoeff_dec, level, decay_level, start_level);
				state = IDLE;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_dec;
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
		last_reset = reset[s];
	}

	plugin->last_gate = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->last_reset = last_reset;
	plugin->level = level;
	plugin->state = state;
	plugin->samples = samples;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/omins/adenv_lvl",
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
