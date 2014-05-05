/*
  An LV2 plugin to simulate an analogue style step sequencer.
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

#include <stdio.h>
#include <stdlib.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "math_func.h"
#include "common.h"

#define SEQUENCER_GATE              0
#define SEQUENCER_TRIGGER           1
#define SEQUENCER_LOOP_POINT        2
#define SEQUENCER_RESET             3
#define SEQUENCER_VALUE_GATE_CLOSED 4
#define SEQUENCER_VALUE_START       5
#define SEQUENCER_OUTPUT            (SEQUENCER_MAX_INPUTS + 5)

typedef struct {
	const float* gate;
	const float* trigger;
	const float* loop_steps;
	const float* reset;
	const float* value_gate_closed;
	const float* values[SEQUENCER_MAX_INPUTS];
	float*       output;
	float        srate;
	float        inv_srate;
	float        last_gate;
	float        last_trigger;
	float        last_value;
	unsigned int step_index;
} Sequencer;

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
	Sequencer* plugin = (Sequencer*)instance;

	switch (port) {
	case SEQUENCER_GATE:
		plugin->gate = (const float*)data;
		break;
	case SEQUENCER_TRIGGER:
		plugin->trigger = (const float*)data;
		break;
	case SEQUENCER_LOOP_POINT:
		plugin->loop_steps = (const float*)data;
		break;
	case SEQUENCER_OUTPUT:
		plugin->output = (float*)data;
		break;
	case SEQUENCER_RESET:
		plugin->reset = (const float*)data;
		break;
	case SEQUENCER_VALUE_GATE_CLOSED:
		plugin->value_gate_closed = (const float*)data;
		break;
	default:
		if (port >= SEQUENCER_VALUE_START && port < SEQUENCER_OUTPUT) {
			plugin->values[port - SEQUENCER_VALUE_START] = (const float*)data;
		}
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Sequencer* plugin = (Sequencer*)malloc(sizeof(Sequencer));
	if (!plugin) {
		return NULL;
	}

	plugin->srate     = (float)sample_rate;
	plugin->inv_srate = 1.0f / plugin->srate;

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	Sequencer* plugin = (Sequencer*)instance;

	plugin->last_gate    = 0.0f;
	plugin->last_trigger = 0.0f;
	plugin->last_value   = 0.0f;
	plugin->step_index   = 0;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sequencer* plugin = (Sequencer*)instance;

	/* Gate */
	const float* gate = plugin->gate;

	/* Step Trigger */
	const float* trigger = plugin->trigger;

	/* Loop Steps */
	const float loop_steps = *(plugin->loop_steps);

	/* Reset to Value on Gate Close */
	const float reset = *(plugin->reset);

	/* Value used when gate closed */
	float value_gate_closed = *(plugin->value_gate_closed);

	/* Step Values */
	float values[SEQUENCER_MAX_INPUTS];

	/* Output */
	float* output = plugin->output;

	float last_gate    = plugin->last_gate;
	float last_trigger = plugin->last_trigger;
	float last_value   = plugin->last_value;

	unsigned int  step_index = plugin->step_index;
	unsigned int  loop_index = LRINTF(loop_steps);
	int           rst        = reset > 0.0f;
	int           i;

	loop_index = loop_index == 0 ?  1 : loop_index;
	loop_index = (loop_index > SEQUENCER_MAX_INPUTS)
		? SEQUENCER_MAX_INPUTS
		: loop_index;

	for (i = 0; i < SEQUENCER_MAX_INPUTS; i++) {
		values[i] = *(plugin->values[i]);
	}

	for (uint32_t s = 0; s < sample_count; ++s) {
		if (gate[s] > 0.0f) {
			if (trigger[s] > 0.0f && !(last_trigger > 0.0f)) {
				if (last_gate > 0.0f) {
					step_index++;
					if (step_index >= loop_index) {
						step_index = 0;
					}
				} else {
					step_index = 0;
				}
			}

			output[s] = values[step_index];

			last_value = values[step_index];
		} else {
			if (rst) {
				output[s] = value_gate_closed;
			} else {
				output[s] = last_value;
			}

			step_index = 0;
		}
		last_gate    = gate[s];
		last_trigger = trigger[s];
	}

	plugin->last_gate    = last_gate;
	plugin->last_trigger = last_trigger;
	plugin->last_value   = last_value;
	plugin->step_index   = step_index;
}

static const LV2_Descriptor descriptor = {
	SEQUENCER_URI,
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
