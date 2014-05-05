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

#ifndef MACHINA_GUI_HPP
#define MACHINA_GUI_HPP

#include <string>

#include <gtkmm.h>

#include "raul/Maid.hpp"
#include "raul/TimeStamp.hpp"

#include "machina/types.hpp"
#include "machina_config.h"

using namespace std;

namespace machina {

class Machine;
class Engine;
class Evolver;
class Controller;

namespace client {
class ClientModel;
class ClientObject;
}

namespace gui {

class MachinaCanvas;

class MachinaGUI
{
public:
	explicit MachinaGUI(SPtr<machina::Engine> engine);
	~MachinaGUI();

	SPtr<MachinaCanvas>       canvas()     { return _canvas; }
	SPtr<machina::Engine>     engine()     { return _engine; }
	SPtr<machina::Controller> controller() { return _controller; }
	Forge&                    forge()      { return _forge; }
	SPtr<Raul::Maid>          maid()       { return _maid; }
	Gtk::Window*              window()     { return _main_window; }

	void attach();
	void quit() { _main_window->hide(); }

	bool chain_mode() const { return _chain_mode; }

	double default_length() const {
		return 1 / (double)_quantize_spinbutton->get_value();
	}

	inline void queue_refresh() { _refresh = true; }

	void on_new_object(SPtr<machina::client::ClientObject> object);
	void on_erase_object(SPtr<machina::client::ClientObject> object);

	SPtr<machina::client::ClientModel> client_model() {
		return _client_model;
	}

protected:
	void menu_file_quit();
	void menu_file_open();
	void menu_file_save();
	void menu_file_save_as();
	void menu_import_midi();
	void menu_export_midi();
	void menu_export_graphviz();
	void on_zoom_in();
	void on_zoom_out();
	void on_zoom_normal();
	void show_toolbar_toggled();
	void show_labels_toggled();
	void menu_help_about();
	void menu_help_help();
	void arrange();
	void load_target_clicked();

	void random_mutation(SPtr<machina::Machine> machine);
	void mutate(SPtr<machina::Machine> machine, unsigned mutation);
	void update_toolbar();
	void rebuild_canvas();

	bool scrolled_window_event(GdkEvent* ev);
	bool idle_callback();

#ifdef HAVE_EUGENE
	void evolve_toggled();
	bool evolve_callback();
#endif

	void stop_toggled();
	void play_toggled();
	void record_toggled();
	void step_record_toggled();

	void chain_toggled();
	void fan_toggled();

	void quantize_record_changed();
	void quantize_changed();
	void tempo_changed();

	string _save_uri;
	string _target_filename;

	Raul::TimeUnit _unit;

	SPtr<MachinaCanvas>                _canvas;
	SPtr<machina::Engine>              _engine;
	SPtr<machina::client::ClientModel> _client_model;
	SPtr<machina::Controller>          _controller;

	SPtr<Raul::Maid>       _maid;
	SPtr<machina::Evolver> _evolver;

	Forge _forge;

	Gtk::Main* _gtk_main;

	Gtk::Window*           _main_window;
	Gtk::Dialog*           _help_dialog;
	Gtk::AboutDialog*      _about_window;
	Gtk::Toolbar*          _toolbar;
	Gtk::MenuItem*         _menu_file_open;
	Gtk::MenuItem*         _menu_file_save;
	Gtk::MenuItem*         _menu_file_save_as;
	Gtk::MenuItem*         _menu_file_quit;
	Gtk::MenuItem*         _menu_zoom_in;
	Gtk::MenuItem*         _menu_zoom_out;
	Gtk::MenuItem*         _menu_zoom_normal;
	Gtk::MenuItem*         _menu_view_arrange;
	Gtk::MenuItem*         _menu_import_midi;
	Gtk::MenuItem*         _menu_export_midi;
	Gtk::MenuItem*         _menu_export_graphviz;
	Gtk::MenuItem*         _menu_help_about;
	Gtk::CheckMenuItem*    _menu_view_labels;
	Gtk::CheckMenuItem*    _menu_view_toolbar;
	Gtk::MenuItem*         _menu_help_help;
	Gtk::ScrolledWindow*   _canvas_scrolledwindow;
	Gtk::TextView*         _status_text;
	Gtk::Expander*         _messages_expander;
	Gtk::SpinButton*       _bpm_spinbutton;
	Gtk::CheckButton*      _quantize_checkbutton;
	Gtk::SpinButton*       _quantize_spinbutton;
	Gtk::ToggleToolButton* _stop_button;
	Gtk::ToggleToolButton* _play_button;
	Gtk::ToggleToolButton* _record_button;
	Gtk::ToggleToolButton* _step_record_button;
	Gtk::RadioButton*      _chain_button;
	Gtk::RadioButton*      _fan_button;
	Gtk::ToolButton*       _load_target_button;
	Gtk::Toolbar*          _evolve_toolbar;
	Gtk::ToggleToolButton* _evolve_button;
	Gtk::ToolButton*       _mutate_button;
	Gtk::ToolButton*       _compress_button;
	Gtk::ToolButton*       _add_node_button;
	Gtk::ToolButton*       _remove_node_button;
	Gtk::ToolButton*       _adjust_node_button;
	Gtk::ToolButton*       _add_edge_button;
	Gtk::ToolButton*       _remove_edge_button;
	Gtk::ToolButton*       _adjust_edge_button;

	bool _refresh;
	bool _evolve;
	bool _chain_mode;
};

}  // namespace machina
}  // namespace gui

#endif // MACHINA_GUI_HPP
