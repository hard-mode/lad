/* This file is part of Eugene
 * Copyright 2007-2012 David Robillard <http://drobilla.net>
 *
 * Eugene is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Eugene is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Eugene.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EUGENE_TSPCANVAS_HPP
#define EUGENE_TSPCANVAS_HPP

#include <string>
#include <boost/shared_ptr.hpp>
#include "ganv/ganv.hpp"

#include "../src/TSP.hpp"

namespace eugene {

namespace GUI {

class TSPCanvas : public Ganv::Canvas {
public:
	static std::shared_ptr<TSPCanvas> load(std::shared_ptr<TSP> tsp);

	void update(const TSP::GeneType& best);

private:
	TSPCanvas(double width, double height) : Ganv::Canvas(width, height) {}

	std::vector< std::shared_ptr<Ganv::Node> > _nodes;
};

} // namespace GUI
} // namespace eugene

#endif  // EUGENE_TSPCANVAS_HPP
