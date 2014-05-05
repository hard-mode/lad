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

#include <algorithm>
#include <bitset>
#include <sstream>
#include <cstring>
#include <fstream>
#include <ctime>

#include <unistd.h>

#include "eugene/Crossover.hpp"
#include "eugene/GA.hpp"
#include "eugene/Gene.hpp"
#include "eugene/Mutation.hpp"
#include "eugene/Mutation.hpp"
#include "eugene/OnePointCrossover.hpp"
#include "eugene/OrderCrossover.hpp"
#include "eugene/PartiallyMappedCrossover.hpp"
#include "eugene/PositionBasedCrossover.hpp"
#include "eugene/Problem.hpp"
#include "eugene/Random.hpp"
#include "eugene/RouletteSelection.hpp"
#include "eugene/TournamentSelection.hpp"
#include "eugene/TwoPointCrossover.hpp"
#include "eugene/UniformCrossover.hpp"

#include "LABS.hpp"
#include "TSP.hpp"

using namespace std;
using namespace eugene;

struct RandomIntGene : public Gene<uint32_t>
{
	RandomIntGene(Random& rng, size_t size, uint32_t min, uint32_t max)
		: Gene<uint32_t>(size, 0)
	{
		for (size_t i = 0; i < size; ++i)
			(*this)[i] = rng.integer(min, max);
	}
};

class OneMax : public Problem<RandomIntGene> {
public:
	typedef RandomIntGene GeneType;

	explicit OneMax(size_t gene_size) : Problem<GeneType>(gene_size) {}

	void initial_population(Random&     rng,
	                        Population& pop,
	                        size_t      gene_size,
	                        size_t      pop_size) const {
		pop.reserve(pop_size);
		while (pop.size() < pop_size)
			pop.push_back(GeneType(rng, gene_size, 0, 1));
	}

	float evaluate(const GeneType& g) const {
		uint32_t sum = 0;
		for (size_t i = 0; i < g.size(); ++i)
			sum += g[i];

		return sum / (float)g.size();
	}

	bool fitness_less_than(float a, float b) const { return a < b; }

#ifndef NDEBUG
	virtual bool assert_gene(const GeneType& g) const {
		for (size_t i = 0; i < g.size(); ++i) {
			if (g[i] != 0 && g[i] != 1)
				return false;
		}

		return true;
	}
#endif
};

class SimpleMax : public Problem<RandomIntGene> {
public:
	typedef RandomIntGene GeneType;

	SimpleMax() : Problem<GeneType>(10) {}

	void initial_population(Random&     rng,
	                        Population& pop,
	                        size_t      gene_size,
	                        size_t      pop_size) const {
		pop.reserve(pop_size);
		while (pop.size() < pop_size) {
			pop.push_back(GeneType(rng, gene_size, 1, 10));
		}
	}

	float evaluate(const GeneType& g) const {
		assert(g.size() == 10);

		return ( (g[0] * g[1] * g[2] * g[3] * g[4])
			/  (float)(g[5] * g[6] * g[7] * g[8] * g[9]) );
	}

	bool fitness_less_than(float a, float b) const { return a < b; }

#ifndef NDEBUG
	virtual bool assert_gene(const GeneType& g) const { return true; }
#endif
};

template <typename G>
void
print_children(const std::pair<G,G>& children)
{
	cout << "\tChild 1: ";
	for (typename G::const_iterator i=children.first.begin(); i != children.first.end(); ++i)
		std::cout << *i << ",";

	cout << endl << "\tChild 2: ";
	for (typename G::const_iterator i=children.second.begin(); i != children.second.end(); ++i)
		std::cout << *i << ",";

	cout << endl;
}

static bool key_pressed()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}

template<typename G>
void
run(GA<G>* ga, bool quiet, bool limit_generations, int limit)
{
	float best_fitness = ga->best_fitness();
	if (!quiet) {
		cout << endl << "Initial best: " << ga->best() << endl;
	}

	while ( ! key_pressed()) {
		ga->iteration();
		float this_best_fitness = ga->best_fitness();
		if (!quiet) {
			if (this_best_fitness != best_fitness) {
				cout << endl << "Generation " << ga->generation() << ": " << ga->best();
				best_fitness = this_best_fitness;
			}
		}

		if (ga->optimum_known()) {
			if (best_fitness == ga->optimum()) {
				cout << "Reached optimum" << endl;
				break;
			}
		}

		if (limit > 0) {
			if (limit_generations && ga->generation() >= limit)
				break;
			else if (!limit_generations && ga->evaluations() >= limit)
				break;
		}
	}

	if (!quiet)
		cout << endl << endl;

	cout << "Finished at generation " << ga->generation() << endl;
	cout << "Best individual: " << ga->best() << endl;
	if (ga->optimum_known())
		cout << "(Optimum " << ga->optimum() << ")" << endl;

	/*
	ofstream ofs;
	ofs.open("population.txt");
	ga->print_population(ofs);
	ofs.close();
	*/
}

int
main(int argc, char** argv)
{
	Random rng(time(NULL));

	enum Mode { ONE_MAX, SIMPLE_MAX, TSP, CROSSOVER, LABS };
	Mode mode = ONE_MAX;

	// Least robust argument handling ever
	if (argc > 2 && !strcmp(argv[1], "--tsp")) {
		mode = TSP;
	} else if ((argc >= 6 && argc <= 9) && !strcmp(argv[1], "--labs")) {
		mode = LABS;
	} else if (argc > 1 && !strcmp(argv[1], "--one-max")) {
		mode = ONE_MAX;
	} else if (argc > 1 && !strcmp(argv[1], "--simple-max")) {
		mode = SIMPLE_MAX;
	} else if (argc > 1 && !strcmp(argv[1], "--crossover")) {
		mode = CROSSOVER;
	} else {
		cerr << "USAGE: " << argv[0] << " --one-max"      << endl
			 << "       " << argv[0] << " --simple-max"   << endl
			 << "       " << argv[0] << " --tsp FILENAME" << endl
			 << "       " << argv[0] << " --crossover"    << endl
		     << "       " << argv[0] << " --labs N P M [ 1 | 2 | U ] [ G | L ] L [--quiet]" << endl;

		return -1;
	}

	size_t population_size      = 1000;
	float  mutation_probability = 0.1;

	int limit = 0;
	bool limit_generations = true; // otherwise evaluations
	bool quiet = false;

	if (mode == LABS) {
		quiet = (argc > 8);

		if (!quiet)
			cout << "Problem size: " << argv[2] << endl;

		population_size = atoi(argv[3]);
		mutation_probability = atof(argv[4]);

		limit = atoi(argv[7]);
		if (argc > 6) {
			if (argv[6][0] == 'L') {
				if (!quiet)
					cout << "Limit: " << limit << " evaluations" << endl;
				limit_generations = false;
			} else if (!quiet) {
					cout << "Limit: " << limit << " generations" << endl;
			}
		}
	}

	if (!quiet) {
		cout << "Population Size:      " << population_size << endl;
		cout << "Mutation Probability: " << mutation_probability << endl;
	}

	if (limit == 0 && !quiet)
		cout << "(Press enter to terminate)" << endl;

	// 1) a)
	if (mode == ONE_MAX) {
		typedef GA<OneMax::GeneType> OneMaxGA;

		std::shared_ptr< Problem<OneMax::GeneType> > p(new OneMax(50));
		std::shared_ptr< Selection<OneMax::GeneType> > s(
			new RouletteSelection<OneMax::GeneType>(*p.get()));
		std::shared_ptr< Crossover<OneMax::GeneType> > c(
				new OnePointCrossover<OneMax::GeneType>());
		std::shared_ptr< Mutation<SimpleMax::GeneType> > m(
				new RandomMutation<SimpleMax::GeneType>(0, 1));

		SimpleMax::Population pop;
		p->initial_population(rng, pop, 50, population_size);
		OneMaxGA ga(rng, p, s, c, m, pop, 50, population_size, 5, mutation_probability, 1.0);
		run(&ga, quiet, limit_generations, limit);

	// 1) b)
	} else if (mode == SIMPLE_MAX) {
		typedef GA<SimpleMax::GeneType> SimpleMaxGA;

		std::shared_ptr< Problem<SimpleMax::GeneType> > p(new SimpleMax());
		std::shared_ptr< Selection<SimpleMax::GeneType> > s(
			new RouletteSelection<SimpleMax::GeneType>(*p.get()));
		std::shared_ptr< Crossover<SimpleMax::GeneType> > c(
				new OnePointCrossover<SimpleMax::GeneType>());
		std::shared_ptr< Mutation<SimpleMax::GeneType> > m(
				new RandomMutation<SimpleMax::GeneType>(1, 10));

		SimpleMax::Population pop;
		p->initial_population(rng, pop, 10, population_size);
		SimpleMaxGA ga(rng, p, s, c, m, pop, 10, population_size, 5, mutation_probability, 1.0);
		run(&ga, quiet, limit_generations, limit);

	} else if (mode == TSP) {
		typedef GA<TSP::GeneType> TSPGA;

		std::shared_ptr< Problem<eugene::TSP::GeneType> > p(
			new eugene::TSP(argv[2]));
		std::shared_ptr< Selection<TSP::GeneType> > s(
			new TournamentSelection<TSP::GeneType>(*p.get(), 4, 0.8));
		std::shared_ptr< Crossover<TSP::GeneType> > c(
				//new OrderCrossover<TSP::GeneType>());
				//new PositionBasedCrossover<TSP::GeneType>());
				new PartiallyMappedCrossover<TSP::GeneType>());
		std::shared_ptr< Mutation<TSP::GeneType> > m(
				new SwapAlleleMutation<TSP::GeneType>());

		TSP::Population pop;
		p->initial_population(rng, pop, p->gene_size(), population_size);
		TSPGA ga(rng, p, s, c, m, pop, p->gene_size(), population_size, 5, mutation_probability, 1.0);
		run(&ga, quiet, limit_generations, limit);

	} else if (mode == LABS) {
		typedef GA<LABS::GeneType> LABSGA;

		size_t gene_size = atoi(argv[2]);
		std::shared_ptr< Problem<eugene::LABS::GeneType> > p(
			new eugene::LABS(gene_size));
		std::shared_ptr< Selection<LABS::GeneType> > s(
			new TournamentSelection<LABS::GeneType>(*p.get(), 3, 0.95));
		std::shared_ptr< Crossover<LABS::GeneType> > c;
		switch (argv[5][0]) {
		case '1':
			if (!quiet)
				cout << "Using one point crossover" << endl;
			c = std::shared_ptr< Crossover<LABS::GeneType> >(
					new OnePointCrossover<LABS::GeneType>());
			break;
		case '2':
			if (!quiet)
				cout << "Using two point crossover" << endl;
			c = std::shared_ptr< Crossover<LABS::GeneType> >(
					new TwoPointCrossover<LABS::GeneType>());
			break;
		case 'U':
			if (!quiet)
				cout << "Using uniform crossover" << endl;
			c = std::shared_ptr< Crossover<LABS::GeneType> >(
					new UniformCrossover<LABS::GeneType>());
			break;
		default:
			cerr << "Unknown crossover, exiting" << endl;
			return -1;
		}

		LABS::Population pop;
		p->initial_population(rng, pop, p->gene_size(), population_size);
		std::shared_ptr< Mutation<LABS::GeneType> > m(
				//new FlipMutation<LABS::GeneType>());
				//new FlipRangeMutation<LABS::GeneType>());
				new FlipRandomMutation<LABS::GeneType>());
		LABSGA ga(rng, p, s, c, m, pop, p->gene_size(), population_size, 3, mutation_probability, 0.8);
		run(&ga, quiet, limit_generations, limit);

	} else if (mode == CROSSOVER) {
		/*string p1_str, p2_str;
		cout << "Enter parent 1: ";
		getline(cin, p1_str);
		cout << endl << "Enter parent 2: ";
		getline(cin, p2_str);
		cout << endl;*/

		std::shared_ptr< Problem<eugene::TSP::GeneType> > p(new eugene::TSP());

		string p1_str = "0 2 4 6 8 1 3 5 7 9";
		string p2_str = "9 8 7 6 5 4 3 2 1 0";

		TSP::GeneType p1(0);
		std::istringstream p1_ss(p1_str);
		while (!p1_ss.eof()) {
			size_t val;
			p1_ss >> val;
			p1.push_back(val);
		}
		assert(p->assert_gene(p1));

		TSP::GeneType p2(0);
		std::istringstream p2_ss(p2_str);
		while (!p2_ss.eof()) {
			size_t val;
			p2_ss >> val;
			p2.push_back(val);
		}
		assert(p->assert_gene(p2));

		std::pair<TSP::GeneType,TSP::GeneType> children
			= make_pair(TSP::GeneType(0), TSP::GeneType(0));

		cout << "Single point:" << endl;
		std::shared_ptr< Crossover<TSP::GeneType> > c(
				new OnePointCrossover<TSP::GeneType>());
		children = c->crossover(rng, p1, p2);
		print_children(children);

		cout << endl << "Partially mapped:" << endl;
		c = std::shared_ptr< Crossover<TSP::GeneType> >(
				new PartiallyMappedCrossover<TSP::GeneType>());
		children = c->crossover(rng, p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));

		cout << endl << "Order based:" << endl;
		c = std::shared_ptr< Crossover<TSP::GeneType> >(
				new OrderCrossover<TSP::GeneType>());
		children = c->crossover(rng, p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));

		cout << endl << "Position based:" << endl;
		c = std::shared_ptr< Crossover<TSP::GeneType> >(
				new PositionBasedCrossover<TSP::GeneType>());
		children = c->crossover(rng, p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));

		return 0;

	} else {
		cerr << "Unknown mode.  Exiting." << endl;
		return -1;
	}

	//cout << endl << "Population written to population.txt" << endl;

	//cout << "Final Elites:" << endl;
	//ga->print_elites();

	return 0;
}
