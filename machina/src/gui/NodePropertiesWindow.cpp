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

#include <string>

#include "client/ClientObject.hpp"
#include "machina/URIs.hpp"
#include "raul/TimeStamp.hpp"

#include "MachinaGUI.hpp"
#include "NodePropertiesWindow.hpp"
#include "WidgetFactory.hpp"

using namespace std;

namespace machina {
namespace gui {

NodePropertiesWindow* NodePropertiesWindow::_instance = NULL;

NodePropertiesWindow::NodePropertiesWindow(
	BaseObjectType*                   cobject,
	const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::Dialog(cobject)
	, _gui(NULL)
{
	property_visible() = false;

	xml->get_widget("node_properties_note_spinbutton", _note_spinbutton);
	xml->get_widget("node_properties_duration_spinbutton", _duration_spinbutton);
	xml->get_widget("node_properties_apply_button", _apply_button);
	xml->get_widget("node_properties_cancel_button", _cancel_button);
	xml->get_widget("node_properties_ok_button", _ok_button);

	_apply_button->signal_clicked().connect(
		sigc::mem_fun(this, &NodePropertiesWindow::apply_clicked));
	_cancel_button->signal_clicked().connect(
		sigc::mem_fun(this, &NodePropertiesWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(
		sigc::mem_fun(this, &NodePropertiesWindow::ok_clicked));
}

NodePropertiesWindow::~NodePropertiesWindow()
{}

void
NodePropertiesWindow::apply_clicked()
{
#if 0
	const uint8_t note = _note_spinbutton->get_value();
	if (!_node->enter_action()) {
		_node->set_enter_action(ActionFactory::note_on(note));
		_node->set_exit_action(ActionFactory::note_off(note));
	} else {
		SPtr<MidiAction> action = dynamic_ptr_cast<MidiAction>(_node->enter_action());
		action->event()[1] = note;
		action             = dynamic_ptr_cast<MidiAction>(_node->exit_action());
		action->event()[1] = note;
	}
#endif
	_node->set(URIs::instance().machina_duration,
	           _gui->forge().make(float(_duration_spinbutton->get_value())));
}

void
NodePropertiesWindow::cancel_clicked()
{
	assert(this == _instance);
	delete _instance;
	_instance = NULL;
}

void
NodePropertiesWindow::ok_clicked()
{
	apply_clicked();
	cancel_clicked();
}

void
NodePropertiesWindow::set_node(MachinaGUI*                         gui,
                               SPtr<machina::client::ClientObject> node)
{
	_gui  = gui;
	_node = node;
	#if 0
	SPtr<MidiAction> enter_action = dynamic_ptr_cast<MidiAction>(node->enter_action());
	if (enter_action && ( enter_action->event_size() > 1)
	    && ( (enter_action->event()[0] & 0xF0) == 0x90) ) {
		_note_spinbutton->set_value(enter_action->event()[1]);
		_note_spinbutton->show();
	} else if (!enter_action) {
		_note_spinbutton->set_value(60);
		_note_spinbutton->show();
	} else {
		_note_spinbutton->hide();
	}
	#endif
	_duration_spinbutton->set_value(
		node->get(URIs::instance().machina_duration).get<float>());
}

void
NodePropertiesWindow::present(MachinaGUI*                         gui,
                              Gtk::Window*                        parent,
                              SPtr<machina::client::ClientObject> node)
{
	if (!_instance) {
		Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create();

		xml->get_widget_derived("node_properties_dialog", _instance);

		if (parent) {
			_instance->set_transient_for(*parent);
		}
	}

	_instance->set_node(gui, node);
	_instance->show();
}

}  // namespace machina
}  // namespace gui
