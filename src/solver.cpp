// solver.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <boost/program_options.hpp>

#include "rps.h"
#include "blotto.h"
#include "kuhn.h"


int main(int argc, char** argv) {
	namespace po = boost::program_options;
	po::options_description desc("game solver");

	std::string game;
	int iters;

	desc.add_options()
		("game", po::value<std::string>(&game)->required())
		("iters", po::value<int>(&iters)->default_value(1000000));

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch (const std::exception& ex) {
		std::cout << desc << std::endl;
		return 1;
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	std::cout << "solving: " << game << "\n";

	if (game == "rps") {
		rps_t rps{ iters };
		rps.train();
	} else if (game == "blotto") {
		constexpr size_t num_soldiers = 5;
		constexpr size_t num_battlefields = 3;
		bf::blotto_t<num_soldiers, num_battlefields> blotto;

		blotto.train(iters);
	} else if (game == "kuhn") {
		kuhn::solver_t solver;
		solver.train(iters);

		solver.show();
	}
}

