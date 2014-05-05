/*
  An LV2 plugin to generate ADSR envelopes Gate and Trigger variant.
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
#include "common.h"

#define ADSR_GATE    0
#define ADSR_TRIGGER 1
#define ADSR_ATTACK  2
#define ADSR_DECAY   3
#define ADSR_SUSTAIN 4
#define ADSR_RELEASE 5
#define ADSR_OUTPUT  6

typedef enum {
	IDLE,
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE
} ADSRState;

typedef struct {
	const float* gate;
	const float* trigger;
	const float* attack;
	const float* decay;
	const float* sustain;
	const float* release;
	float*       output;
	float        srate;
	float        inv_srate;
	float        last_trigger;
	float        from_level;
	float        level;
	ADSRState    state;
	uint32_t     samples;
} Adsr;

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
	Adsr* plugin = (Adsr*)instance;

	switch (port) {
	case ADSR_GATE:
		plugin->gate = (const float*)data;
		break;
	case ADSR_TRIGGER:
		plugin->trigger = (const float*)data;
		break;
	case ADSR_ATTACK:
		plugin->attack = (const float*)data;
		break;
	case ADSR_DECAY:
		plugin->decay = (const float*)data;
		break;
	case ADSR_SUSTAIN:
		plugin->sustain = (const float*)data;
		break;
	case ADSR_RELEASE:
		plugin->release = (const float*)data;
		break;
	case ADSR_OUTPUT:
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
	Adsr* plugin = (Adsr*)malloc(sizeof(Adsr));

	if (plugin) {
		plugin->srate     = (float)sample_rate;
		plugin->inv_srate = 1.0f / plugin->srate;
	}

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Adsr* plugin = (Adsr*)instance;

	plugin->last_trigger = 0.0f;
	plugin->from_level   = 0.0f;
	plugin->level        = 0.0f;
	plugin->state        = IDLE;
	plugin->samples      = 0;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Adsr* plugin = (Adsr*)instance;

	/* Gate */
	const float* gate = plugin->gate;

	/* Trigger */
	const float* trigger = plugin->trigger;

	/* Attack Time (s) */
	float attack = *(plugin->attack);

	/* Decay Time (s) */
	float decay = *(plugin->decay);

	/* Sustain Level */
	const float sustain = f_clip(*(plugin->sustain), 0.0f, 1.0f);

	/* Release Time (s) */
	float release = *(plugin->release);

	/* Envelope Out */
	float* output = plugin->output;

	float     srate        = plugin->srate;
	float     inv_srate    = plugin->inv_srate;
	float     last_trigger = plugin->last_trigger;
	float     from_level   = plugin->from_level;
	float     level        = plugin->level;
	ADSRState state        = plugin->state;
	uint32_t  samples      = plugin->samples;

	float elapsed;

	/* Convert times into rates */
	attack  = attack > 0.0f ? inv_srate / attack : srate;
	decay   = decay > 0.0f ? inv_srate / decay : srate;
	release = release > 0.0f ? inv_srate / release : srate;

	for (uint32_t s = 0; s < sample_count; ++s) {
		/* Attack on trigger, if gate is open */
		if (trigger[s] > 0.0f
		    && !(last_trigger > 0.0f)
		    && gate[s] > 0.0f) {
			if (attack < srate) {
				state = ATTACK;
			} else {
				state = decay < srate ? DECAY : SUSTAIN;
				level = 1.0f;
			}
			samples = 0;
		}

		/* Release if gate closed */
		if (state != IDLE
		    && state != RELEASE
		    && !(gate[s] > 0.0f)) {
			state   = release < srate ? RELEASE : IDLE;
			samples = 0;
		}

		if (samples == 0) {
			from_level = level;
		}

		/* Calculate level of envelope from current state */
		switch (state) {
		case IDLE:
			level = 0;
			break;
		case ATTACK:
			samples++;
			elapsed = (float)samples * attack;
			if (elapsed > 1.0f) {
				state   = decay < srate ? DECAY : SUSTAIN;
				level   = 1.0f;
				samples = 0;
			} else {
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case DECAY:
			samples++;
			elapsed = (float)samples * decay;
			if (elapsed > 1.0f) {
				state   = SUSTAIN;
				level   = sustain;
				samples = 0;
			} else {
				level = from_level + elapsed * (sustain - from_level);
			}
			break;
		case SUSTAIN:
			level = sustain;
			break;
		case RELEASE:
			samples++;
			elapsed = (float)samples * release;
			if (elapsed > 1.0f) {
				state   = IDLE;
				level   = 0.0f;
				samples = 0;
			} else {
				level = from_level - elapsed * from_level;
			}
			break;
		default:
			/* Should never happen */
			level = 0.0f;
		}

		output[s]    = level;
		last_trigger = trigger[s];
	}

	plugin->last_trigger = last_trigger;
	plugin->from_level   = from_level;
	plugin->level        = level;
	plugin->state        = state;
	plugin->samples      = samples;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/adsr_gt",
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
