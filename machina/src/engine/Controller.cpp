/*
  This file is part of Machina.
  Copyright 2007-2014 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "machina/Controller.hpp"
#include "machina/Engine.hpp"
#include "machina/Machine.hpp"
#include "machina/Updates.hpp"

#include "Edge.hpp"
#include "MidiAction.hpp"

namespace machina {

Controller::Controller(SPtr<Engine> engine,
                       Model&       model)
	: _engine(engine)
	, _model(model)
	, _updates(new Raul::RingBuffer(4096))
{
	_engine->driver()->set_update_sink(_updates);
}

uint64_t
Controller::create(const Properties& properties)
{
	std::map<URIInt, Atom>::const_iterator d = properties.find(
		URIs::instance().machina_duration);

	double duration = 0.0;
	if (d != properties.end()) {
		duration = d->second.get<float>();
	} else {
		std::cerr << "warning: new object has no duration" << std::endl;
	}

	TimeDuration dur(_engine->machine()->time().unit(), duration);
	SPtr<Node> node(new Node(dur));
	_objects.insert(node);
	_model.new_object(node->id(), properties);
	_engine->machine()->add_node(node);
	return node->id();
}

void
Controller::announce(SPtr<Machine> machine)
{
	if (!machine) {
		return;
	}

	Forge& forge = _engine->forge();

	// Announce nodes
	for (const auto& n : machine->nodes()) {
		Properties properties {
			{ URIs::instance().rdf_type,
			  forge.make_urid(URIs::instance().machina_Node) },
			{ URIs::instance().machina_duration,
			  forge.make(float(n->duration().to_double())) } };
		if (n->is_initial()) {
			properties.insert({ URIs::instance().machina_initial,
			                    forge.make(true) });
		}

		SPtr<MidiAction> midi_action = dynamic_ptr_cast<MidiAction>(
			n->enter_action());
		if (midi_action) {
			Properties action_properties {
				{ URIs::instance().machina_note_number,
				  forge.make((int32_t)midi_action->event()[1]) } };

			_model.new_object(midi_action->id(), action_properties);
			properties.insert({ URIs::instance().machina_enter_action,
			                    forge.make(int32_t(n->enter_action()->id())) });
		}

		_objects.insert(n);
		_model.new_object(n->id(), properties);
	}

	// Announce edges
	for (const auto& n : machine->nodes()) {
		for (const auto& e : n->edges()) {
			const Properties properties {
				{ URIs::instance().rdf_type,
				  forge.make_urid(URIs::instance().machina_Edge) },
				{ URIs::instance().machina_probability,
				  forge.make(e->probability()) },
				{ URIs::instance().machina_tail_id,
				  forge.make((int32_t)n->id()) },
				{ URIs::instance().machina_head_id,
				  forge.make((int32_t)e->head()->id()) } };

			_objects.insert(e);
			_model.new_object(e->id(), properties);
		}
	}
}

SPtr<Stateful>
Controller::find(uint64_t id)
{
	SPtr<StatefulKey> key(new StatefulKey(id));
	Objects::iterator i = _objects.find(key);
	if (i != _objects.end()) {
		return *i;
	}
	return SPtr<Stateful>();
}

void
Controller::learn(SPtr<Raul::Maid> maid, uint64_t node_id)
{
	SPtr<Node> node = dynamic_ptr_cast<Node>(find(node_id));
	if (node) {
		_engine->machine()->learn(maid, node);
	} else {
		std::cerr << "Failed to find node " << node_id << " for learn"
		          << std::endl;
	}
}

void
Controller::set_property(uint64_t    object_id,
                         URIInt      key,
                         const Atom& value)
{
	SPtr<Stateful> object = find(object_id);
	if (object) {
		object->set(key, value);
		_model.set(object_id, key, value);
	}
}

uint64_t
Controller::connect(uint64_t tail_id, uint64_t head_id)
{
	SPtr<Node> tail = dynamic_ptr_cast<Node>(find(tail_id));
	SPtr<Node> head = dynamic_ptr_cast<Node>(find(head_id));

	if (!tail) {
		std::cerr << "error: tail node " << tail_id << " not found" << std::endl;
		return 0;
	} else if (!head) {
		std::cerr << "error: head node " << head_id << " not found" << std::endl;
		return 0;
	}

	SPtr<Edge> edge(new Edge(tail, head));
	tail->add_edge(edge);
	_objects.insert(edge);

	Forge& forge = _engine->forge();

	const Properties properties = {
		{ URIs::instance().rdf_type,
		  forge.make_urid(URIs::instance().machina_Edge) },
		{ URIs::instance().machina_probability, forge.make(1.0f) },
		{ URIs::instance().machina_tail_id,
		  forge.make((int32_t)tail->id()) },
		{ URIs::instance().machina_head_id,
		  forge.make((int32_t)head->id()) } };

	_model.new_object(edge->id(), properties);

	return edge->id();
}

void
Controller::disconnect(uint64_t tail_id, uint64_t head_id)
{
	SPtr<Node> tail = dynamic_ptr_cast<Node>(find(tail_id));
	SPtr<Node> head = dynamic_ptr_cast<Node>(find(head_id));

	SPtr<Edge> edge = tail->remove_edge_to(head);
	if (edge) {
		_model.erase_object(edge->id());
	} else {
		std::cerr << "Edge not found" << std::endl;
	}
}

void
Controller::erase(uint64_t id)
{
	SPtr<StatefulKey> key(new StatefulKey(id));
	Objects::iterator i = _objects.find(key);
	if (i == _objects.end()) {
		return;
	}

	SPtr<Node> node = dynamic_ptr_cast<Node>(*i);
	if (node) {
		_engine->machine()->remove_node(node);
	}

	_model.erase_object((*i)->id());
	_objects.erase(i);
}

void
Controller::process_updates()
{
	const uint32_t read_space = _updates->read_space();

	uint64_t subject;
	URIInt   key;
	Atom     value;
	for (uint32_t i = 0; i < read_space; ) {
		i += read_set(_updates, &subject, &key, &value);
		_model.set(subject, key, value);
	}
}

}
