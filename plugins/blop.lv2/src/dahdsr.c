/*
  An LV2 plugin to generate DAHDSR envelopes Gate and (re)trigger
  Copyright 2011 David Robillard
  Copyright 2004 Mike Rawes

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
#include "common.h"
#include "uris.h"

#define DAHDSR_GATE    0
#define DAHDSR_TRIGGER 1
#define DAHDSR_DELAY   2
#define DAHDSR_ATTACK  3
#define DAHDSR_HOLD    4
#define DAHDSR_DECAY   5
#define DAHDSR_SUSTAIN 6
#define DAHDSR_RELEASE 7
#define DAHDSR_OUTPUT  8

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
	const float* gate;
	const float* trigger;
	const float* delay;
	const float* attack;
	const float* hold;
	const float* decay;
	const float* sustain;
	const float* release;
	float*       output;
	float        srate;
	float        inv_srate;
	float        last_gate;
	float        last_trigger;
	float        from_level;
	float        level;
	uint32_t     delay_is_cv;
	uint32_t     attack_is_cv;
	uint32_t     hold_is_cv;
	uint32_t     decay_is_cv;
	uint32_t     sustain_is_cv;
	uint32_t     release_is_cv;
	DAHDSRState  state;
	uint32_t     samples;
	URIs         uris;
} Dahdsr;

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
	Dahdsr* plugin = (Dahdsr*)instance;

	switch (port) {
	case DAHDSR_GATE:
		plugin->gate = (const float*)data;
		break;
	case DAHDSR_TRIGGER:
		plugin->trigger = (const float*)data;
		break;
	case DAHDSR_DELAY:
		plugin->delay = (const float*)data;
		break;
	case DAHDSR_ATTACK:
		plugin->attack = (const float*)data;
		break;
	case DAHDSR_HOLD:
		plugin->hold = (const float*)data;
		break;
	case DAHDSR_DECAY:
		plugin->decay = (const float*)data;
		break;
	case DAHDSR_SUSTAIN:
		plugin->sustain = (const float*)data;
		break;
	case DAHDSR_RELEASE:
		plugin->release = (const float*)data;
		break;
	case DAHDSR_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	Dahdsr*  plugin = (Dahdsr*)instance;
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
			case DAHDSR_DELAY:
				plugin->delay_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case DAHDSR_ATTACK:
				plugin->attack_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case DAHDSR_HOLD:
				plugin->hold_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case DAHDSR_DECAY:
				plugin->decay_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case DAHDSR_SUSTAIN:
				plugin->sustain_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case DAHDSR_RELEASE:
				plugin->release_is_cv = (port_type == plugin->uris.lv2_CVPort);
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
	Dahdsr* plugin = (Dahdsr*)malloc(sizeof(Dahdsr));
	if (!plugin) {
		return NULL;
	}

	plugin->srate     = (float)sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	plugin->delay_is_cv   = 0;
	plugin->attack_is_cv  = 0;
	plugin->hold_is_cv    = 0;
	plugin->decay_is_cv   = 0;
	plugin->sustain_is_cv = 0;
	plugin->release_is_cv = 0;

	map_uris(&plugin->uris, features);

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Dahdsr* plugin = (Dahdsr*)instance;

	plugin->last_gate    = 0.0f;
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
	Dahdsr* plugin = (Dahdsr*)instance;

	/* Gate */
	const float* gate = plugin->gate;

	/* Trigger */
	const float* trigger = plugin->trigger;

	/* Delay Time (s) */
	const float* delay = plugin->delay;

	/* Attack Time (s) */
	const float* attack = plugin->attack;

	/* Hold Time (s) */
	const float* hold = plugin->hold;

	/* Decay Time (s) */
	const float* decay = plugin->decay;

	/* Sustain Level */
	const float* sustain = plugin->sustain;

	/* Release Time (s) */
	const float* release = plugin->release;

	/* Envelope Out */
	float* output = plugin->output;

	/* Instance Data */
	float       srate        = plugin->srate;
	float       inv_srate    = plugin->inv_srate;
	float       last_gate    = plugin->last_gate;
	float       last_trigger = plugin->last_trigger;
	float       from_level   = plugin->from_level;
	float       level        = plugin->level;
	DAHDSRState state        = plugin->state;
	uint32_t    samples      = plugin->samples;

	float elapsed;

	for (uint32_t s = 0; s < sample_count; ++s) {
		const float dl = delay[s * plugin->delay_is_cv];
		const float at = attack[s * plugin->attack_is_cv];
		const float hl = hold[s * plugin->hold_is_cv];
		const float dc = decay[s * plugin->decay_is_cv];
		const float st = sustain[s * plugin->sustain_is_cv];
		const float rl = release[s * plugin->release_is_cv];

		/* Convert times into rates */
		const float del = dl > 0.0f ? inv_srate / dl : srate;
		const float att = at > 0.0f ? inv_srate / at : srate;
		const float hld = hl > 0.0f ? inv_srate / hl : srate;
		const float dec = dc > 0.0f ? inv_srate / dc : srate;
		const float rel = rl > 0.0f ? inv_srate / rl : srate;

		const float gat = gate[s];
		const float trg = trigger[s];
		const float sus = f_clip(st, 0.0f, 1.0f);

		/* Initialise delay phase if gate is opened and was closed, or
		   we received a trigger */
		if ((trg > 0.0f && !(last_trigger > 0.0f))
		    || (gat > 0.0f && !(last_gate > 0.0f))) {
			if (del < srate) {
				state = DELAY;
			} else if (att < srate) {
				state = ATTACK;
			} else {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE
							 : IDLE)));
				level = 1.0f;
			}
			samples = 0;
		}

		/* Release if gate was open and now closed */
		if (state != IDLE && state != RELEASE
		    && last_gate > 0.0f && !(gat > 0.0f)) {
			state   = rel < srate ? RELEASE : IDLE;
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
		case DELAY:
			samples++;
			elapsed = (float)samples * del;
			if (elapsed > 1.0f) {
				state = att < srate ? ATTACK
					: (hld < srate ? HOLD
					   : (dec < srate ? DECAY
						  : (gat > 0.0f ? SUSTAIN
							 : (rel < srate ? RELEASE
								: IDLE))));
				samples = 0;
			}
			break;
		case ATTACK:
			samples++;
			elapsed = (float)samples * att;
			if (elapsed > 1.0f) {
				state = hld < srate ? HOLD
					: (dec < srate ? DECAY
					   : (gat > 0.0f ? SUSTAIN
						  : (rel < srate ? RELEASE
							 : IDLE)));
				level   = 1.0f;
				samples = 0;
			} else {
				level = from_level + elapsed * (1.0f - from_level);
			}
			break;
		case HOLD:
			samples++;
			elapsed = (float)samples * hld;
			if (elapsed > 1.0f) {
				state = dec < srate ? DECAY
					: (gat > 0.0f ? SUSTAIN
					   : (rel < srate ? RELEASE
						  : IDLE));
				samples = 0;
			}
			break;
		case DECAY:
			samples++;
			elapsed = (float)samples * dec;
			if (elapsed > 1.0f) {
				state = gat > 0.0f ? SUSTAIN
					: (rel < srate ? RELEASE
					   : IDLE);
				level   = sus;
				samples = 0;
			} else {
				level = from_level + elapsed * (sus - from_level);
			}
			break;
		case SUSTAIN:
			level = sus;
			break;
		case RELEASE:
			samples++;
			elapsed = (float)samples * rel;
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

		output[s] = level;

		last_gate    = gate[s];
		last_trigger = trigger[s];
	}

	plugin->last_gate    = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->from_level   = from_level;
	plugin->level        = level;
	plugin->state        = state;
	plugin->samples      = samples;
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
	"http://drobilla.net/plugins/blop/dahdsr",
	instantiate,
	connect_port,
	activate,
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
