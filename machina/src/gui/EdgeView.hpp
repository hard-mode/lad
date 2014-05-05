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

#ifndef MACHINA_EDGEVIEW_HPP
#define MACHINA_EDGEVIEW_HPP

#include "ganv/Edge.hpp"

#include "client/ClientObject.hpp"

#include "machina/types.hpp"

namespace machina {
namespace gui {

class NodeView;

class EdgeView
	: public Ganv::Edge
	, public machina::client::ClientObject::View
{
public:
	EdgeView(Ganv::Canvas&                       canvas,
	         NodeView*                           src,
	         NodeView*                           dst,
	         SPtr<machina::client::ClientObject> edge);

	~EdgeView();

	void show_label(bool show);

	virtual double length_hint() const;

private:
	bool on_event(GdkEvent* ev);
	void on_property(machina::URIInt key, const Atom& value);

	float probability() const;

	SPtr<machina::client::ClientObject> _edge;
};

}  // namespace machina
}  // namespace gui

#endif // MACHINA_EDGEVIEW_HPP
