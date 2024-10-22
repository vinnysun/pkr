// solver.cpp : Defines the entry point for the application.
//

#include "rps.h"
#include "blotto.h"
#include <iostream>

using namespace std;

int main()
{
	cout << "Hello CMake." << endl;

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
	blotto.train(1'000'0000);


	std::cout << std::endl;
	return 0;
}
