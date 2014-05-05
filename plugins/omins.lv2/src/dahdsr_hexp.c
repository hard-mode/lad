/*
    dahdsr.so.c - A LV2 plugin to generate DAHDSR envelopes
                  linear attack, exponential decay and release version.
    Copyright 2005 Loki Davison, based on DAHDSR by Mike Rawes

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
#include <math.h>

#define DAHDSR_GATE                      0
#define DAHDSR_TRIGGER                   1
#define DAHDSR_DELAY                     2
#define DAHDSR_ATTACK                    3
#define DAHDSR_HOLD                      4
#define DAHDSR_DECAY                     5
#define DAHDSR_SUSTAIN                   6
#define DAHDSR_RELEASE                   7
#define DAHDSR_OUTPUT                    8

LV2_Descriptor **dahdsr_descriptors = 0;

typedef enum {
	IDLE,
	DELAY,
	ATTACK,
	HOLD,
	DECAY,
	SUSTAIN,
	RELEASE
} DAHDSRState;

typedef struct {
	float *gate;
	float *trigger;
	float *delay;
	float *attack;
	float *hold;
	float *decay;
	float *sustain;
	float *release;
	float *output;
	float srate;
	float inv_srate;
	float last_gate;
	float last_trigger;
	float from_level;
	float level;
	DAHDSRState state;
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
	case DAHDSR_GATE:
		plugin->gate = data;
		break;
	case DAHDSR_TRIGGER:
		plugin->trigger = data;
		break;
	case DAHDSR_DELAY:
		plugin->delay = data;
		break;
	case DAHDSR_ATTACK:
		plugin->attack = data;
		break;
	case DAHDSR_HOLD:
		plugin->hold = data;
		break;
	case DAHDSR_DECAY:
		plugin->decay = data;
		break;
	case DAHDSR_SUSTAIN:
		plugin->sustain = data;
		break;
	case DAHDSR_RELEASE:
		plugin->release = data;
		break;
	case DAHDSR_OUTPUT:
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

	/* Delay Time (s) */
	float delay = *(plugin->delay);

	/* Attack Time (s) */
	float attack = *(plugin->attack);

	/* Hold Time (s) */
	float hold = *(plugin->hold);

	/* Decay Time (s) */
	float decay = *(plugin->decay);

	/* Sustain Level */
	float sustain = *(plugin->sustain);

	/* Release Time (s) */
	float release = *(plugin->release);

	/* Envelope Out */
	float *output = plugin->output;

	/* Instance Data */
	float srate = plugin->srate;
	float inv_srate = plugin->inv_srate;
	float last_gate = plugin->last_gate;
	float last_trigger = plugin->last_trigger;
	float from_level = plugin->from_level;
	float level = plugin->level;
	DAHDSRState state = plugin->state;
	unsigned long samples = plugin->samples;

	float gat, trg, del, att, hld, dec, sus, rel;
	float elapsed;
	unsigned long s;

	/* Convert times into rates */
	del = delay > 0.0f ? inv_srate / delay : srate;
	att = attack > 0.0f ? inv_srate / attack : srate;
	hld = hold > 0.0f ? inv_srate / hold : srate;
	dec = decay > 0.0f ? inv_srate / decay : srate;
	rel = release > 0.0f ? inv_srate / release : srate;
	sus = sustain;

	if (sus == 0) {
		sus = 0.001;
	}
	if (sus > 1.0f) {
		sus = 1.0f;
	}

	//float ReleaseCoeff_att = (0 - log(0.001)) / (attack * srate);
	float ReleaseCoeff_dec = (log(sus)) / (decay * srate);
	float ReleaseCoeff_rel =
		(log(0.001) - log(sus)) / (release * srate);

	for (s = 0; s < sample_count; s++) {
		gat = gate[s];
		trg = trigger[s];

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f)) ||
			(gat > 0.0f && !(last_gate > 0.0f))) {
			if (del < srate) {
				state = DELAY;
			} else if (att < srate) {
				state = ATTACK;
			} else {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE : IDLE)));
				level = 1.0f;
			}
			samples = 0;
		}

		/* Release if gate was open and now closed */
		if (state != IDLE && state != RELEASE &&
			last_gate > 0.0f && !(gat > 0.0f)) {
			state = rel < srate ? RELEASE : IDLE;
			samples = 0;
		}

		if (samples == 0)
			from_level = level;

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			level = 0;
			break;
		case DELAY:
			samples++;
			elapsed = (float) samples *del;

			if (elapsed > 1.0f) {
				state = att < srate ? ATTACK
					: (hld < srate ? HOLD
					   : (dec < srate ? DECAY
						  : (gat > 0.0f ? SUSTAIN
							 : (rel < srate ? RELEASE : IDLE))));
				samples = 0;
			}
			break;
		case ATTACK:
			samples++;
			elapsed = (float) samples *att;

			if (elapsed > 1.0f) {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE : IDLE)));
				level = 1.0f;
				samples = 0;
			} else {
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case HOLD:
			samples++;
			elapsed = (float) samples *hld;

			if (elapsed > 1.0f) {
				state = dec < srate ? DECAY
					: (gat > 0.0f ? SUSTAIN : (rel < srate ? RELEASE : IDLE));
				samples = 0;
			}
			break;
		case DECAY:
			samples++;
			elapsed = (float) samples *dec;

			if (elapsed > 1.0f) {
				state = gat > 0.0f ? SUSTAIN : (rel < srate ? RELEASE : IDLE);
				level = sus;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_dec;
			}
			break;
		case SUSTAIN:
			level = sus;
			break;
		case RELEASE:
			samples++;
			elapsed = (float) samples *rel;

			if (elapsed > 1.0f) {
				state = IDLE;
				level = 0.0f;
				samples = 0;
			} else {
				level += level * ReleaseCoeff_rel;
			}
			break;
		default:
			/* Should never happen */
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
	"http://drobilla.net/plugins/omins/dahdsr_hexp",
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
