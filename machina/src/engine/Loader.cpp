/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <iostream>
#include <map>

#include <glibmm/ustring.h>

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "machina/Loader.hpp"
#include "machina/URIs.hpp"
#include "machina/Machine.hpp"

#include "Edge.hpp"
#include "MidiAction.hpp"
#include "Node.hpp"
#include "SMFDriver.hpp"
#include "machina_config.h"

using namespace Raul;
using namespace std;

namespace machina {

Loader::Loader(Forge& forge, Sord::World& rdf_world)
	: _forge(forge)
	, _rdf_world(rdf_world)
{}

static SPtr<Action>
load_action(Sord::Model& model, Sord::Node node)
{
	if (!node.is_valid()) {
		return SPtr<Action>();
	}

	Sord::URI rdf_type(model.world(), MACHINA_URI_RDF "type");
	Sord::URI midi_NoteOn(model.world(), LV2_MIDI__NoteOn);
	Sord::URI midi_NoteOff(model.world(), LV2_MIDI__NoteOff);
	Sord::URI midi_noteNumber(model.world(), LV2_MIDI__noteNumber);
	Sord::URI midi_velocity(model.world(), LV2_MIDI__velocity);

	Sord::Node type   = model.get(node, rdf_type, Sord::Node());
	uint8_t    status = 0;
	if (type == midi_NoteOn) {
		status = LV2_MIDI_MSG_NOTE_ON;
	} else if (type == midi_NoteOff) {
		status = LV2_MIDI_MSG_NOTE_OFF;
	} else {
		return SPtr<Action>();
	}

	Sord::Node num_node = model.get(node, midi_noteNumber, Sord::Node());
	Sord::Node vel_node = model.get(node, midi_velocity, Sord::Node());

	const uint8_t num      = num_node.is_int() ? num_node.to_int() : 64;
	const uint8_t vel      = vel_node.is_int() ? vel_node.to_int() : 64;
	const uint8_t event[3] = { status, num, vel };

	return SPtr<Action>(new MidiAction(sizeof(event), event));
}

/** Load (create) all objects from RDF into the engine.
 *
 * @param uri URI of machine (resolvable URI to an RDF document).
 * @return Loaded Machine.
 */
SPtr<Machine>
Loader::load(const Glib::ustring& uri)
{
	using Glib::ustring;

	ustring document_uri = uri;

	// If "URI" doesn't contain a colon, try to resolve as a filename
	if (uri.find(":") == ustring::npos) {
		document_uri = "file://" + document_uri;
	}

	cout << "Loading " << document_uri << endl;

	TimeUnit beats(TimeUnit::BEATS, MACHINA_PPQN);

	SPtr<Machine> machine(new Machine(beats));

	typedef std::map<Sord::Node, SPtr<Node> > Created;
	Created created;

	Sord::URI   base_uri(_rdf_world, document_uri);
	Sord::Model model(_rdf_world, document_uri);

	SerdEnv* env = serd_env_new(base_uri.to_serd_node());
	model.load_file(env, SERD_TURTLE, document_uri);
	serd_env_free(env);

	Sord::Node nil;

	Sord::URI machina_SelectorNode(_rdf_world, MACHINA_NS_SelectorNode);
	Sord::URI machina_duration(_rdf_world, MACHINA_NS_duration);
	Sord::URI machina_edge(_rdf_world, MACHINA_NS_arc);
	Sord::URI machina_head(_rdf_world, MACHINA_NS_head);
	Sord::URI machina_node(_rdf_world, MACHINA_NS_node);
	Sord::URI machina_onEnter(_rdf_world, MACHINA_NS_onEnter);
	Sord::URI machina_onExit(_rdf_world, MACHINA_NS_onExit);
	Sord::URI machina_probability(_rdf_world, MACHINA_NS_probability);
	Sord::URI machina_start(_rdf_world, MACHINA_NS_start);
	Sord::URI machina_tail(_rdf_world, MACHINA_NS_tail);
	Sord::URI rdf_type(_rdf_world, MACHINA_URI_RDF "type");

	Sord::Node subject = base_uri;

	// Get start node ID (but re-use existing start node)
	Sord::Iter i = model.find(subject, machina_start, nil);
	if (i.end()) {
		cerr << "error: Machine has no start node" << std::endl;
	}
	created[i.get_object()] = machine->initial_node();

	// Get remaining nodes
	for (Sord::Iter i = model.find(subject, machina_node, nil); !i.end(); ++i) {
		const Sord::Node& id = i.get_object();
		if (created.find(id) != created.end()) {
			cerr << "warning: Machine lists the same node twice" << std::endl;
			continue;
		}

		// Create node
		Sord::Iter d = model.find(id, machina_duration, nil);
		SPtr<Node> node(new Node(TimeStamp(beats, d.get_object().to_float())));
		machine->add_node(node);
		created[id] = node;

		node->set_enter_action(
			load_action(model, model.get(id, machina_onEnter, nil)));
		node->set_exit_action(
			load_action(model, model.get(id, machina_onExit, nil)));
	}

	// Get arcs
	for (Sord::Iter i = model.find(subject, machina_edge, nil); !i.end(); ++i) {
		Sord::Node edge = i.get_object();
		Sord::Iter t    = model.find(edge, machina_tail, nil);
		Sord::Iter h    = model.find(edge, machina_head, nil);
		Sord::Iter p    = model.find(edge, machina_probability, nil);

		Sord::Node tail        = t.get_object();
		Sord::Node head        = h.get_object();
		Sord::Node probability = p.get_object();

		float prob = probability.to_float();

		Created::iterator tail_i = created.find(tail);
		Created::iterator head_i = created.find(head);

		if (tail_i != created.end() && head_i != created.end()) {
			const SPtr<Node> tail = tail_i->second;
			const SPtr<Node> head = head_i->second;
			tail->add_edge(SPtr<Edge>(new Edge(tail, head, prob)));
		} else {
			cerr << "warning: Ignored edge between unknown nodes "
			     << tail << " -> " << head << endl;
		}
	}

	if (machine && !machine->nodes().empty()) {
		machine->reset(NULL, machine->time());
		return machine;
	} else {
		return SPtr<Machine>();
	}
}

SPtr<Machine>
Loader::load_midi(const Glib::ustring& uri,
                  double               q,
                  Raul::TimeDuration   dur)
{
	SPtr<SMFDriver> file_driver(new SMFDriver(_forge, dur.unit()));
	return file_driver->learn(uri, q, dur);
}

} // namespace machina
