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

#include "machina_config.h"

#include <cmath>
#include <fstream>
#include <limits.h>
#include <pthread.h>
#include "sord/sordmm.hpp"
#include "machina/Controller.hpp"
#include "machina/Engine.hpp"
#include "machina/Machine.hpp"
#include "machina/Mutation.hpp"
#include "machina/Updates.hpp"
#include "client/ClientModel.hpp"
#include "WidgetFactory.hpp"
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"
#include "EdgeView.hpp"

#ifdef HAVE_EUGENE
#include "machina/Evolver.hpp"
#endif

namespace machina {
namespace gui {

MachinaGUI::MachinaGUI(SPtr<machina::Engine> engine)
	: _unit(TimeUnit::BEATS, 19200)
	, _engine(engine)
	, _client_model(new machina::client::ClientModel())
	, _controller(new machina::Controller(_engine, *_client_model.get()))
	, _maid(new Raul::Maid())
	, _refresh(false)
	, _evolve(false)
	, _chain_mode(true)
{
	_canvas = SPtr<MachinaCanvas>(new MachinaCanvas(this, 1600*2, 1200*2));

	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create();

	xml->get_widget("machina_win", _main_window);
	xml->get_widget("about_win", _about_window);
	xml->get_widget("help_dialog", _help_dialog);
	xml->get_widget("toolbar", _toolbar);
	xml->get_widget("open_menuitem", _menu_file_open);
	xml->get_widget("save_menuitem", _menu_file_save);
	xml->get_widget("save_as_menuitem", _menu_file_save_as);
	xml->get_widget("quit_menuitem", _menu_file_quit);
	xml->get_widget("zoom_in_menuitem", _menu_zoom_in);
	xml->get_widget("zoom_out_menuitem", _menu_zoom_out);
	xml->get_widget("zoom_normal_menuitem", _menu_zoom_normal);
	xml->get_widget("arrange_menuitem", _menu_view_arrange);
	xml->get_widget("import_midi_menuitem", _menu_import_midi);
	xml->get_widget("export_midi_menuitem", _menu_export_midi);
	xml->get_widget("export_graphviz_menuitem", _menu_export_graphviz);
	xml->get_widget("view_toolbar_menuitem", _menu_view_toolbar);
	xml->get_widget("view_labels_menuitem", _menu_view_labels);
	xml->get_widget("help_about_menuitem", _menu_help_about);
	xml->get_widget("help_help_menuitem", _menu_help_help);
	xml->get_widget("canvas_scrolledwindow", _canvas_scrolledwindow);
	xml->get_widget("bpm_spinbutton", _bpm_spinbutton);
	xml->get_widget("quantize_checkbutton", _quantize_checkbutton);
	xml->get_widget("quantize_spinbutton", _quantize_spinbutton);
	xml->get_widget("stop_but", _stop_button);
	xml->get_widget("play_but", _play_button);
	xml->get_widget("record_but", _record_button);
	xml->get_widget("step_record_but", _step_record_button);
	xml->get_widget("chain_but", _chain_button);
	xml->get_widget("fan_but", _fan_button);
	xml->get_widget("load_target_but", _load_target_button);
	xml->get_widget("evolve_toolbar", _evolve_toolbar);
	xml->get_widget("evolve_but", _evolve_button);
	xml->get_widget("mutate_but", _mutate_button);
	xml->get_widget("compress_but", _compress_button);
	xml->get_widget("add_node_but", _add_node_button);
	xml->get_widget("remove_node_but", _remove_node_button);
	xml->get_widget("adjust_node_but", _adjust_node_button);
	xml->get_widget("add_edge_but", _add_edge_button);
	xml->get_widget("remove_edge_but", _remove_edge_button);
	xml->get_widget("adjust_edge_but", _adjust_edge_button);

	_canvas_scrolledwindow->add(_canvas->widget());
	_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(this,
				&MachinaGUI::scrolled_window_event));

	_canvas_scrolledwindow->property_hadjustment().get_value()->set_step_increment(10);
	_canvas_scrolledwindow->property_vadjustment().get_value()->set_step_increment(10);

	_stop_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::stop_toggled));
	_play_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::play_toggled));
	_record_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::record_toggled));
	_step_record_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::step_record_toggled));

	_chain_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::chain_toggled));
	_fan_button->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::fan_toggled));

	_menu_file_open->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_open));
	_menu_file_save->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save));
	_menu_file_save_as->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save_as));
	_menu_file_quit->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_quit));
	_menu_zoom_in->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::on_zoom_in));
	_menu_zoom_out->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::on_zoom_out));
	_menu_zoom_normal->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::on_zoom_normal));
	_menu_view_arrange->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::arrange));
	_menu_import_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_import_midi));
	_menu_export_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_midi));
	_menu_export_graphviz->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_graphviz));
	_menu_view_toolbar->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_toolbar_toggled));
	_menu_view_labels->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_labels_toggled));
	_menu_help_about->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_about));
	_menu_help_help->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_help));
	_bpm_spinbutton->signal_changed().connect(
		sigc::mem_fun(this, &MachinaGUI::tempo_changed));
	_quantize_checkbutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_record_changed));
	_quantize_spinbutton->signal_changed().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_changed));

	_mutate_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::random_mutation),
		           SPtr<Machine>()));
	_compress_button->signal_clicked().connect(
		sigc::hide_return(sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		                             SPtr<Machine>(), 0)));
	_add_node_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 1));
	_remove_node_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 2));
	_adjust_node_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 3));
	_add_edge_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 4));
	_remove_edge_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 5));
	_adjust_edge_button->signal_clicked().connect(
		sigc::bind(sigc::mem_fun(this, &MachinaGUI::mutate),
		           SPtr<Machine>(), 6));

	_canvas->widget().show();

	_main_window->present();

	_quantize_checkbutton->set_active(false);
	update_toolbar();

	// Idle callback to drive the maid (collect garbage)
	Glib::signal_timeout().connect(
		sigc::bind_return(sigc::mem_fun(_maid.get(), &Raul::Maid::cleanup),
		                  true),
		1000);

	// Idle callback to update node states
	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &MachinaGUI::idle_callback), 100);

#ifdef HAVE_EUGENE
	_load_target_button->signal_clicked().connect(
		sigc::mem_fun(this, &MachinaGUI::load_target_clicked));
	_evolve_button->signal_clicked().connect(
		sigc::mem_fun(this, &MachinaGUI::evolve_toggled));
	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &MachinaGUI::evolve_callback), 1000);
#else
	_evolve_toolbar->hide();
#endif

	_client_model->signal_new_object.connect(
		sigc::mem_fun(this, &MachinaGUI::on_new_object));
	_client_model->signal_erase_object.connect(
		sigc::mem_fun(this, &MachinaGUI::on_erase_object));

	rebuild_canvas();
}

MachinaGUI::~MachinaGUI()
{
}

#ifdef HAVE_EUGENE
bool
MachinaGUI::evolve_callback()
{
	if (_evolve && _evolver->improvement()) {
		_engine->driver()->set_machine(
			SPtr<Machine>(new Machine(_evolver->best())));
		_controller->announce(_engine->machine());
	}

	return true;
}
#endif

bool
MachinaGUI::idle_callback()
{
	_controller->process_updates();
	return true;
}

static void
destroy_edge(GanvEdge* edge, void* data)
{
	MachinaGUI* gui  = (MachinaGUI*)data;
	EdgeView*   view = dynamic_cast<EdgeView*>(Glib::wrap(edge));
	if (view) {
		NodeView* tail = dynamic_cast<NodeView*>(view->get_tail());
		NodeView* head = dynamic_cast<NodeView*>(view->get_head());
		gui->controller()->disconnect(tail->node()->id(), head->node()->id());
	}
}

static void
destroy_node(GanvNode* node, void* data)
{
	MachinaGUI* gui  = (MachinaGUI*)data;
	NodeView*   view = dynamic_cast<NodeView*>(Glib::wrap(GANV_NODE(node)));
	if (view) {
		const SPtr<client::ClientObject> node = view->node();
		gui->canvas()->for_each_edge_on(
			GANV_NODE(view->gobj()), destroy_edge, gui);
		gui->controller()->erase(node->id());
	}
}

bool
MachinaGUI::scrolled_window_event(GdkEvent* event)
{
	if (event->type == GDK_KEY_PRESS) {
		if (event->key.keyval == GDK_Delete) {
			_canvas->for_each_selected_node(destroy_node, this);
			return true;
		}
	}

	return false;
}

void
MachinaGUI::arrange()
{
	_canvas->arrange();
}

void
MachinaGUI::load_target_clicked()
{
	Gtk::FileChooserDialog dialog(*_main_window,
			"Load MIDI file for evolution", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_local_only(false);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK)
		_target_filename = dialog.get_filename();
}

#ifdef HAVE_EUGENE
void
MachinaGUI::evolve_toggled()
{
	if (_evolve_button->get_active()) {
		_evolver = SPtr<Evolver>(
			new Evolver(_unit, _target_filename, _engine->machine()));
		_evolve = true;
		stop_toggled();
		_engine->driver()->set_machine(SPtr<Machine>());
		_evolver->start();
	} else {
		_evolver->join();
		_evolve = false;
		SPtr<Machine> new_machine = SPtr<Machine>(
			new Machine(_evolver->best()));
		_engine->driver()->set_machine(new_machine);
		_controller->announce(_engine->machine());
		_engine->driver()->activate();
	}
}
#endif

void
MachinaGUI::random_mutation(SPtr<Machine> machine)
{
	if (!machine)
		machine = _engine->machine();

	mutate(machine, machine->nodes().size() < 2 ? 1 : rand() % 7);
}

void
MachinaGUI::mutate(SPtr<Machine> machine, unsigned mutation)
{
	#if 0
	if (!machine)
		machine = _engine->machine();

	using namespace Mutation;

	switch (mutation) {
		case 0:
			Compress().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 1:
			AddNode().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 2:
			RemoveNode().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 3:
			AdjustNode().mutate(*machine.get());
			idle_callback(); // update nodes
			break;
		case 4:
			AddEdge().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 5:
			RemoveEdge().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 6:
			AdjustEdge().mutate(*machine.get());
			_canvas->update_edges();
			break;
		default: throw;
	}
	#endif
}

void
MachinaGUI::update_toolbar()
{
	const Driver::PlayState state = _engine->driver()->play_state();
	_record_button->set_active(state == Driver::PlayState::RECORDING);
	_step_record_button->set_active(state == Driver::PlayState::STEP_RECORDING);
	_play_button->set_active(state == Driver::PlayState::PLAYING);
}

void
MachinaGUI::rebuild_canvas()
{
	_controller->announce(_engine->machine());
	_canvas->arrange();
}

void
MachinaGUI::quantize_record_changed()
{
	_engine->driver()->set_quantize_record(_quantize_checkbutton->get_active());
}

void
MachinaGUI::quantize_changed()
{
	_engine->set_quantization(1.0 / _quantize_spinbutton->get_value());
}

void
MachinaGUI::tempo_changed()
{
	_engine->set_bpm(_bpm_spinbutton->get_value_as_int());
}

void
MachinaGUI::menu_file_quit()
{
	_main_window->hide();
}

void
MachinaGUI::menu_file_open()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Open Machine", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_local_only(false);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filt;
	filt.add_pattern("*.machina.ttl");
	filt.set_name("Machina Machines (Turtle/RDF)");
	dialog.set_filter(filt);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SPtr<machina::Machine> new_machine = _engine->load_machine(dialog.get_uri());
		if (new_machine) {
			rebuild_canvas();
			_save_uri = dialog.get_uri();
		}
	}
}

void
MachinaGUI::menu_file_save()
{
	if (_save_uri == "" || _save_uri.substr(0, 5) != "file:") {
		menu_file_save_as();
	} else {
		if (_save_uri.substr(0, 5) != "file:")
			menu_file_save_as();

		Sord::Model model(_engine->rdf_world(), _save_uri);
		_engine->machine()->write_state(model);
		model.write_to_file(_save_uri, SERD_TURTLE);
	}
}

void
MachinaGUI::menu_file_save_as()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Save Machine",
			Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	if (_save_uri.length() > 0)
		dialog.set_uri(_save_uri);

	const int result = dialog.run();

	assert(result == Gtk::RESPONSE_OK
			|| result == Gtk::RESPONSE_CANCEL
			|| result == Gtk::RESPONSE_NONE);

	if (result == Gtk::RESPONSE_OK) {
		string filename = dialog.get_filename();

		if (filename.length() < 13 || filename.substr(filename.length()-12) != ".machina.ttl")
			filename += ".machina.ttl";

		Glib::ustring uri = Glib::filename_to_uri(filename);

		bool confirm = false;
		std::fstream fin;
		fin.open(filename.c_str(), std::ios::in);
		if (fin.is_open()) {  // File exists
			string msg = "A file named \"";
			msg += filename + "\" already exists.\n\nDo you want to replace it?";
			Gtk::MessageDialog confirm_dialog(dialog,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confirm_dialog.run() == Gtk::RESPONSE_YES)
				confirm = true;
			else
				confirm = false;
		} else {  // File doesn't exist
			confirm = true;
		}
		fin.close();

		if (confirm) {
			_save_uri = uri;
			Sord::Model model(_engine->rdf_world(), _save_uri);
			_engine->machine()->write_state(model);
			model.write_to_file(_save_uri, SERD_TURTLE);
		}
	}
}

void
MachinaGUI::menu_import_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Learn from MIDI file",
			Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);

	Gtk::HBox* extra_widget = Gtk::manage(new Gtk::HBox());
	Gtk::SpinButton* length_sb = Gtk::manage(new Gtk::SpinButton());
	length_sb->set_increments(1, 10);
	length_sb->set_range(0, INT_MAX);
	length_sb->set_value(0);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("")), true, true);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("Maximum Length (0 = unlimited): ")), false, false);
	extra_widget->pack_start(*length_sb, false, false);
	dialog.set_extra_widget(*extra_widget);
	extra_widget->show_all();

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		const double length_dbl = length_sb->get_value_as_int();
		const Raul::TimeStamp length(_unit, length_dbl);

		SPtr<machina::Machine> machine = _engine->load_machine_midi(
			dialog.get_filename(), 0.0, length);

		if (machine) {
			dialog.hide();
			machine->reset(NULL, machine->time());
			_engine->driver()->set_machine(machine);
			_canvas->clear();
			rebuild_canvas();
		} else {
			Gtk::MessageDialog msg_dialog(dialog, "Error loading MIDI file",
					false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			msg_dialog.run();
		}
	}
}

void
MachinaGUI::menu_export_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a MIDI file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);

	Gtk::HBox* extra_widget = Gtk::manage(new Gtk::HBox());
	Gtk::SpinButton* dur_sb = Gtk::manage(new Gtk::SpinButton());
	dur_sb->set_increments(1, 10);
	dur_sb->set_range(0, INT_MAX);
	dur_sb->set_value(0);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("")), true, true);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("Duration (beats): ")), false, false);
	extra_widget->pack_start(*dur_sb, false, false);
	dialog.set_extra_widget(*extra_widget);
	extra_widget->show_all();

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		const double dur_dbl = dur_sb->get_value_as_int();
		const Raul::TimeStamp dur(_unit, dur_dbl);
		_engine->export_midi(dialog.get_filename(), dur);
	}
}

void
MachinaGUI::menu_export_graphviz()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a GraphViz DOT file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK)
		_canvas->export_dot(dialog.get_filename().c_str());
}

void
MachinaGUI::show_toolbar_toggled()
{
	if (_menu_view_toolbar->get_active())
		_toolbar->show();
	else
		_toolbar->hide();
}

void
MachinaGUI::on_zoom_in()
{
	_canvas->set_font_size(_canvas->get_font_size() + 1.0);
}

void
MachinaGUI::on_zoom_out()
{
	_canvas->set_font_size(_canvas->get_font_size() - 1.0);
}

void
MachinaGUI::on_zoom_normal()
{
	_canvas->set_zoom(1.0);
}

static void
show_node_label(GanvNode* node, void* data)
{
	bool            show   = *(bool*)data;
	Ganv::Node*     nodemm = Glib::wrap(node);
	NodeView* const nv     = dynamic_cast<NodeView*>(nodemm);
	if (nv) {
		nv->show_label(show);
	}
}

static void
show_edge_label(GanvEdge* edge, void* data)
{
	bool            show   = *(bool*)data;
	Ganv::Edge*     edgemm = Glib::wrap(edge);
	EdgeView* const ev     = dynamic_cast<EdgeView*>(edgemm);
	if (ev) {
		ev->show_label(show);
	}
}

void
MachinaGUI::show_labels_toggled()
{
	bool show = _menu_view_labels->get_active();

	_canvas->for_each_node(show_node_label, &show);
	_canvas->for_each_edge(show_edge_label, &show);
}

void
MachinaGUI::menu_help_about()
{
	_about_window->set_transient_for(*_main_window);
	_about_window->show();
}

void
MachinaGUI::menu_help_help()
{
	_help_dialog->set_transient_for(*_main_window);
	_help_dialog->run();
	_help_dialog->hide();
}

void
MachinaGUI::stop_toggled()
{
	if (_stop_button->get_active()) {
		const Driver::PlayState old_state = _engine->driver()->play_state();
		_engine->driver()->set_play_state(Driver::PlayState::STOPPED);
		if (old_state == Driver::PlayState::RECORDING ||
		    old_state == Driver::PlayState::STEP_RECORDING) {
			rebuild_canvas();
		}
	}
}

void
MachinaGUI::play_toggled()
{
	if (_play_button->get_active()) {
		const Driver::PlayState old_state = _engine->driver()->play_state();
		_engine->driver()->set_play_state(Driver::PlayState::PLAYING);
		if (old_state == Driver::PlayState::RECORDING ||
		    old_state == Driver::PlayState::STEP_RECORDING) {
			rebuild_canvas();
		}
	}
}

void
MachinaGUI::record_toggled()
{
	if (_record_button->get_active()) {
		_engine->driver()->set_play_state(Driver::PlayState::RECORDING);
	}
}

void
MachinaGUI::step_record_toggled()
{
	if (_step_record_button->get_active()) {
		_engine->driver()->set_play_state(Driver::PlayState::STEP_RECORDING);
	}
}

void
MachinaGUI::chain_toggled()
{
	if (_chain_button->get_active()) {
		_chain_mode = true;
	}
}

void
MachinaGUI::fan_toggled()
{
	if (_fan_button->get_active()) {
		_chain_mode = false;
	}
}

void
MachinaGUI::on_new_object(SPtr<client::ClientObject> object)
{
	_canvas->on_new_object(object);
}

void
MachinaGUI::on_erase_object(SPtr<client::ClientObject> object)
{
	_canvas->on_erase_object(object);
}

}  // namespace machina
}  // namespace gui
