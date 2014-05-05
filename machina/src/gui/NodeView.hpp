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

#ifndef MACHINA_NODEVIEW_HPP
#define MACHINA_NODEVIEW_HPP

#include "ganv/Circle.hpp"

#include "client/ClientObject.hpp"

#include "machina/types.hpp"

namespace machina {
namespace gui {

class NodeView
	: public Ganv::Circle
	, public machina::client::ClientObject::View
{
public:
	NodeView(Gtk::Window*                        window,
	         Canvas&                             canvas,
	         SPtr<machina::client::ClientObject> node,
	         double                              x,
	         double                              y);

	~NodeView();

	SPtr<machina::client::ClientObject> node() { return _node; }

	void show_label(bool show);

	void update_state(bool show_labels);

	void set_default_colors();

	sigc::signal<bool, GdkEventButton*>& signal_clicked() {
		return _signal_clicked;
	}

private:
	bool on_event(GdkEvent* ev);
	bool on_double_click(GdkEventButton* ev);
	void on_property(machina::URIInt key, const Atom& value);
	void on_action_property(machina::URIInt key, const Atom& value);

	bool is(Forge& forge, machina::URIInt key);

	Gtk::Window*                        _window;
	SPtr<machina::client::ClientObject> _node;
	uint32_t                            _default_border_color;
	uint32_t                            _default_fill_color;

	SPtr<machina::client::ClientObject> _enter_action;
	sigc::connection                    _enter_action_connection;

	sigc::signal<bool, GdkEventButton*> _signal_clicked;
};

}  // namespace machina
}  // namespace gui

#endif // MACHINA_NODEVIEW_HPP
