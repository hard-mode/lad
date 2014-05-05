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

#ifndef EUGENE_MAINWINDOW_HPP
#define EUGENE_MAINWINDOW_HPP

#include <memory>

#include <gtkmm.h>

#include "UIFile.hpp"
#include "Widget.hpp"

#include "../src/TSP.hpp"

namespace eugene {

template <typename G> class GA;
template <typename G> class HybridCrossover;
template <typename G> class HybridMutation;

namespace GUI {

class TSPCanvas;

class MainWindow {
public:
	MainWindow();

	Glib::RefPtr<Gtk::Builder> xml;

	Widget<Gtk::AboutDialog> about_dialog;
	Widget<Gtk::MenuItem>    about_menuitem;
	Widget<Gtk::Label>       best_generation_label;
	Widget<Gtk::Scale>       crossover_probability_scale;
	Widget<Gtk::SpinButton>  elites_spin;
	Widget<Gtk::Button>      execute_button;
	Widget<Gtk::Label>       generation_label;
	Widget<Gtk::CheckButton> greedy_mutation_check;
	Widget<Gtk::Scale>       injection_scale;
	Widget<Gtk::Window>      main_win;
	Widget<Gtk::Scale>       mutation_scale;
	Widget<Gtk::MenuItem>    open_menuitem;
	Widget<Gtk::Scale>       order_scale;
	Widget<Gtk::Scale>       partially_mapped_scale;
	Widget<Gtk::Scale>       permute_gene_scale;
	Widget<Gtk::SpinButton>  population_spin;
	Widget<Gtk::Scale>       position_based_scale;
	Widget<Gtk::Scale>       random_flip_scale;
	Widget<Gtk::Scale>       random_swap_scale;
	Widget<Gtk::Scale>       reverse_scale;
	Widget<Gtk::ComboBox>    selection_combo;
	Widget<Gtk::Label>       selection_probability_label;
	Widget<Gtk::SpinButton>  selection_probability_spin;
	Widget<Gtk::Label>       shortest_path_label;
	Widget<Gtk::Scale>       single_point_scale;
	Widget<Gtk::Button>      stop_button;
	Widget<Gtk::Scale>       swap_range_scale;
	Widget<Gtk::Label>       trial_size_label;
	Widget<Gtk::SpinButton>  trial_size_spin;
	Widget<Gtk::Viewport>    viewport;

private:
	void ga_run();

	void gtk_on_crossover_changed(size_t index, const Gtk::Scale* scale);
	void gtk_on_mutation_changed(size_t index, const Gtk::Scale* scale);
	void gtk_on_mutation_probability_changed();
	void gtk_on_crossover_probability_changed();
	void gtk_on_num_elites_changed();
	void gtk_on_selection_changed();
	void gtk_on_execute();
	void gtk_on_stop();
	void gtk_on_open();
	void gtk_on_about();
	void gtk_on_help_about();
	bool gtk_iteration();

	Random                                                    _rng;
	std::shared_ptr<eugene::TSP>                              _problem;
	std::shared_ptr< eugene::GA<TSP::GeneType> >              _ga;
	std::shared_ptr< eugene::HybridCrossover<TSP::GeneType> > _crossover;
	std::shared_ptr< eugene::HybridMutation<TSP::GeneType> >  _mutation;
	std::shared_ptr<TSP::GeneType>                            _best;
	std::shared_ptr<TSPCanvas>                                _canvas;
};

} // namespace GUI
} // namespace eugene

#endif  // EUGENE_MAINWINDOW_HPP
