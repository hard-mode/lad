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

#ifndef MACHINA_CANVAS_HPP_HPP
#define MACHINA_CANVAS_HPP_HPP

#include <string>

#include "ganv/Canvas.hpp"
#include "machina/types.hpp"

using namespace Ganv;

namespace machina {

namespace client { class ClientObject; }

namespace gui {

class MachinaGUI;
class NodeView;

class MachinaCanvas : public Canvas
{
public:
	MachinaCanvas(MachinaGUI* app, int width, int height);

	void on_new_object(SPtr<machina::client::ClientObject> object);
	void on_erase_object(SPtr<machina::client::ClientObject> object);

	MachinaGUI* app() { return _app; }

protected:
	bool on_event(GdkEvent* event);

	bool node_clicked(NodeView* node, GdkEventButton* ev);

private:
	void action_create_node(double x, double y);
	void action_connect(NodeView* tail, NodeView* head);
	void action_disconnect(NodeView* tail, NodeView* head);

	static void connect_nodes(GanvNode* node, void* data);

	MachinaGUI* _app;
	NodeView*   _connect_node;
	bool        _did_connect;
};

}  // namespace machina
}  // namespace gui

#endif // MACHINA_CANVAS_HPP_HPP
