﻿// solver.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <boost/program_options.hpp>

#include "rps.h"
#include "blotto.h"


int main(int argc, char** argv) {
	namespace po = boost::program_options;
	po::options_description desc("game solver");

	std::string game;

	desc.add_options()
		("game", po::value<std::string>(&game)->required());

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch (const std::exception& ex) {
		std::cout << desc << std::endl;
		return 1;
	}

	std::cout << "simming: " << game << "\n";

	// rps_t rps{ 100000 };
	// rps.train();

	constexpr size_t num_soldiers = 5;
	constexpr size_t num_battlefields = 3;
	bf::blotto_t<num_soldiers, num_battlefields> blotto;
	/*
	for (int i = 0; i < 1000; i++) {
		std::cout << blotto.rng() << '\n';
	}
	*/
	blotto.train(10'000'000);


	std::cout << std::endl;
	return 0;
}
