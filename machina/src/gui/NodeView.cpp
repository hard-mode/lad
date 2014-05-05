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

#include "machina/Controller.hpp"
#include "machina/URIs.hpp"
#include "machina/types.hpp"

#include "client/ClientModel.hpp"

#include "MachinaCanvas.hpp"
#include "MachinaGUI.hpp"
#include "NodePropertiesWindow.hpp"
#include "NodeView.hpp"

using namespace std;

namespace machina {
namespace gui {

NodeView::NodeView(Gtk::Window*                        window,
                   Ganv::Canvas&                       canvas,
                   SPtr<machina::client::ClientObject> node,
                   double                              x,
                   double                              y)
	: Ganv::Circle(canvas, "", x, y)
	, _window(window)
	, _node(node)
	, _default_border_color(get_border_color())
	, _default_fill_color(get_fill_color())
{
	set_fit_label(false);
	set_radius_ems(1.25);

	signal_event().connect(sigc::mem_fun(this, &NodeView::on_event));

	MachinaCanvas* mcanvas = dynamic_cast<MachinaCanvas*>(&canvas);
	if (is(mcanvas->app()->forge(), URIs::instance().machina_initial)) {
		set_border_width(4.0);
		set_is_source(true);
		const uint8_t alpha[] = { 0xCE, 0xB1, 0 };
		set_label((const char*)alpha);
	}

	node->signal_property.connect(sigc::mem_fun(this, &NodeView::on_property));

	for (const auto& p : node->properties()) {
		on_property(p.first, p.second);
	}
}

NodeView::~NodeView()
{
	_node->set_view(NULL);
}

bool
NodeView::on_double_click(GdkEventButton*)
{
	MachinaCanvas* canvas = dynamic_cast<MachinaCanvas*>(this->canvas());
	NodePropertiesWindow::present(canvas->app(), _window, _node);
	return true;
}

bool
NodeView::is(Forge& forge, machina::URIInt key)
{
	const Atom& value = _node->get(key);
	return value.type() == forge.Bool && value.get<int32_t>();
}

bool
NodeView::on_event(GdkEvent* event)
{
	MachinaCanvas* canvas = dynamic_cast<MachinaCanvas*>(this->canvas());
	Forge&         forge  = canvas->app()->forge();
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button.state & GDK_CONTROL_MASK) {
			if (event->button.button == 1) {
				canvas->app()->controller()->set_property(
					_node->id(),
					URIs::instance().machina_selector,
					forge.make(!is(forge, URIs::instance().machina_selector)));
				return true;
			}
		} else {
			return _signal_clicked.emit(&event->button);
		}
	} else if (event->type == GDK_2BUTTON_PRESS) {
		return on_double_click(&event->button);
	}
	return false;
}

static void
midi_note_name(uint8_t num, uint8_t buf[8])
{
	static const char* notes      = "CCDDEFFGGAAB";
	static const bool  is_sharp[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	const uint8_t octave  = num / 12;
	const uint8_t id      = num - octave * 12;
	const uint8_t sub[]   = { 0xE2, 0x82, uint8_t(0x80 + octave) };
	const uint8_t sharp[] = { 0xE2, 0x99, 0xAF };

	int b    = 0;
	buf[b++] = notes[id];
	if (is_sharp[id]) {
		for (unsigned s = 0; s < sizeof(sharp); ++s) {
			buf[b++] = sharp[s];
		}
	}
	for (unsigned s = 0; s < sizeof(sub); ++s) {
		buf[b++] = sub[s];
	}
	buf[b++] = 0;
}

void
NodeView::show_label(bool show)
{
	if (show && _enter_action) {
		Atom note_number = _enter_action->get(
			URIs::instance().machina_note_number);
		if (note_number.is_valid()) {
			uint8_t buf[8];
			midi_note_name(note_number.get<int32_t>(), buf);
			set_label((const char*)buf);
			return;
		}
	}

	set_label("");
}

void
NodeView::on_property(machina::URIInt key, const Atom& value)
{
	static const uint32_t active_color        = 0x408040FF;
	static const uint32_t active_border_color = 0x00FF00FF;

	if (key == URIs::instance().machina_selector) {
		if (value.get<int32_t>()) {
			set_dash_length(4.0);
		} else {
			set_dash_length(0.0);
		}
	} else if (key == URIs::instance().machina_initial) {
		set_border_width(value.get<int32_t>() ? 4.0 : 1.0);
		set_is_source(value.get<int32_t>());
	} else if (key == URIs::instance().machina_active) {
		if (value.get<int32_t>()) {
			if (get_fill_color() != active_color) {
				set_fill_color(active_color);
				set_border_color(active_border_color);
			}
		} else if (get_fill_color() == active_color) {
			set_default_colors();
		}
	} else if (key == URIs::instance().machina_enter_action) {
		const uint64_t action_id = value.get<int32_t>();
		MachinaCanvas* canvas    = dynamic_cast<MachinaCanvas*>(this->canvas());
		_enter_action_connection.disconnect();
		_enter_action = canvas->app()->client_model()->find(action_id);
		if (_enter_action) {
			_enter_action_connection = _enter_action->signal_property.connect(
				sigc::mem_fun(this, &NodeView::on_action_property));
			for (auto i : _enter_action->properties()) {
				on_action_property(i.first, i.second);
			}
		}
	}
}

void
NodeView::on_action_property(machina::URIInt key, const Atom& value)
{
	if (key == URIs::instance().machina_note_number) {
		show_label(true);
	}
}

void
NodeView::set_default_colors()
{
	set_fill_color(_default_fill_color);
	set_border_color(_default_border_color);
}

}  // namespace machina
}  // namespace gui
