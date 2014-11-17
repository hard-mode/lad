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

#include "sratom/sratom.h"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

enum {
	PORT_INPUT = 0,
};

typedef struct {
	struct {
		const LV2_Atom_Sequence* input;
	} ports;

	Sratom* sratom;

	LV2_URID_Map*   map;
	LV2_URID_Unmap* unmap;
	LV2_Log_Log*    log;

	LV2_Atom_Forge forge;
	LV2_Log_Logger logger;
} Get;

static void
cleanup(LV2_Handle instance)
{
	Get* self = (Get*)instance;
	sratom_free(self->sratom);
	free(self);
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
		} else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
			self->unmap = (LV2_URID_Unmap*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}
	if (!self->map) {
		fprintf(stderr, "Missing feature urid:map.\n");
		free(self);
		return NULL;
	} else if (!self->unmap) {
		fprintf(stderr, "Missing feature urid:unmap.\n");
		free(self);
		return NULL;
	}

	self->sratom = sratom_new(self->map);

	lv2_atom_forge_init(&self->forge, self->map);
	lv2_log_logger_init(&self->logger, self->map, self->log);

	return (LV2_Handle)self;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Get* self = (Get*)instance;

	SerdNode s = serd_node_from_string(SERD_BLANK, (const uint8_t*)"event");
	SerdNode p = serd_node_from_string(SERD_URI, (const uint8_t*)(NS_RDF "value"));

	LV2_ATOM_SEQUENCE_FOREACH(self->ports.input, ev) {
		char* str = sratom_to_turtle(
			self->sratom, self->unmap, "urn:log/", &s, &p,
			ev->body.type, ev->body.size, LV2_ATOM_BODY(&ev->body));
		lv2_log_note(&self->logger, "%s", str);
	}
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/mesp/log",
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
