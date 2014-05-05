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

#ifndef NODEPROPERTIESWINDOW_HPP
#define NODEPROPERTIESWINDOW_HPP

#include <gtkmm.h>

#include "machina/types.hpp"

namespace machina {

namespace client { class ClientObject; }

namespace gui {

class MachinaGUI;

class NodePropertiesWindow : public Gtk::Dialog
{
public:
	NodePropertiesWindow(BaseObjectType*                   cobject,
	                     const Glib::RefPtr<Gtk::Builder>& xml);

	~NodePropertiesWindow();

	static void present(MachinaGUI*                         gui,
	                    Gtk::Window*                        parent,
	                    SPtr<machina::client::ClientObject> node);

private:
	void set_node(MachinaGUI*                         gui,
	              SPtr<machina::client::ClientObject> node);

	void apply_clicked();
	void cancel_clicked();
	void ok_clicked();

	static NodePropertiesWindow* _instance;

	MachinaGUI*                         _gui;
	SPtr<machina::client::ClientObject> _node;

	Gtk::SpinButton* _note_spinbutton;
	Gtk::SpinButton* _duration_spinbutton;
	Gtk::Button*     _apply_button;
	Gtk::Button*     _cancel_button;
	Gtk::Button*     _ok_button;
};

}  // namespace machina
}  // namespace gui

#endif // NODEPROPERTIESWINDOW_HPP
