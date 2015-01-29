/*
  This file is part of Mesp.
  Copyright 2012 David Robillard <http://drobilla.net>

  Mesp is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Mesp is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Mesp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

enum {
	PORT_INPUT    = 0,
	PORT_VALUES   = 1,
	PORT_PROPERTY = 2
};

typedef struct {
	struct {
		const LV2_Atom_Sequence* input;
		LV2_Atom_Sequence*       values;
		const LV2_Atom_URID*     property;
	} ports;

	LV2_URID_Map* map;

	LV2_Atom_Forge forge;
} Get;

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
	Get* self = (Get*)instance;

	switch (port) {
	case PORT_INPUT:
		self->ports.input = (const LV2_Atom_Sequence*)data;
		break;
	case PORT_VALUES:
		self->ports.values = (LV2_Atom_Sequence*)data;
		break;
	case PORT_PROPERTY:
		self->ports.property = (const LV2_Atom_URID*)data;
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Get* self = (Get*)calloc(1, sizeof(Get));
	if (!self) {
		return NULL;
	}

	/* Get host features */
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}
	if (!self->map) {
		fprintf(stderr, "Missing feature urid:map.\n");
		free(self);
		return NULL;
	}

	lv2_atom_forge_init(&self->forge, self->map);

	return (LV2_Handle)self;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Get* self = (Get*)instance;

	/* Set up forge to write directly to values output. */
	const uint32_t values_capacity = self->ports.values->atom.size;
	lv2_atom_forge_set_buffer(&self->forge,
	                          (uint8_t*)self->ports.values,
	                          values_capacity);

	/* Start a sequence in values output. */
	LV2_Atom_Forge_Frame values_frame;
	lv2_atom_forge_sequence_head(&self->forge, &values_frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(self->ports.input, ev) {
		if (lv2_atom_forge_is_object_type(&self->forge, ev->body.type)) {
			const LV2_Atom_Object* obj   = (const LV2_Atom_Object*)&ev->body;
			const LV2_Atom*        value = NULL;
			lv2_atom_object_get(obj, self->ports.property->body, &value, NULL);
			if (value) {
				/* Input object has specified property, emit value. */
				lv2_atom_forge_frame_time(&self->forge, ev->time.frames);
				lv2_atom_forge_write(
					&self->forge, value, lv2_atom_total_size(value));
			}
		}
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/mesp/get",
	instantiate,
	connect_port,
	NULL,
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
