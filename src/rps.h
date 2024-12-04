// solver.h : Include file for standard system include files,
// or project specific include files.

#ifndef rps_h__
#define rps_h__

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <array>
#include <numeric>

/*
make object for actions, here enum
for strategy, need arrays of	
	regret sum - accumulated action regrets
	strategy - generated through regret matching
		vector of probabilities for each action
	strategy sum - sum of all strats generated

regret matching selects actions in proportion to positive regrets of not having chosen them in the past
	base strat reset to max(regrets[i], 0)
	first get sum of all positive regrets (normalizing sum)
	if norm sum > 0, divide all in base strat by norm so that all the postive ones sum to 1 (others are 0)
		these are clamped to 0 at the start
	else (norm sum <= 0, but can only be 0), make strategy uniform (1 / num actions)
	
	accumulate that normed prob to a sum of probs for that action over all training iters
		add the normed sum to vector of strategy sums (index by index)

so with vector of probs, we select one randomly based on their proportion. this is done by generating a
	randon number and "indexing" into this vecto with it
	get_action() below

to train, make vector for utility of each action
for each iter
	get regret matched mixed strategy actions (get_strategy())
		the thing above, literally just get the mixed strategy for the action
		using the same strategy doesn't imply using the same action

	compute utility for each action from the perspective of hero
		current code is rps specific, rps = 0,1,2
		utility of playing same action as villian is 0
		p>r, s>p, r>s means that action + 1 is the winner so utility for that is 1
		and then -1 for the one left of villian action

	accumulate action regrets
		for each action add our regret to cumulative regret for that action
			utility of action a - utility of action we took
			if v played rock, action utility vector looks like [0, +1, -1] (rock tie, paper win, scissors lose)
			let's say we played scissors, then regrets look like [1, 2, 0]
				since we lost, we regret 1 for rock since rock ties, and regret 2 for paper since paper wins
			let's say we played paper, then regrets look like [-1, 0, =2]
				since we won, regret -1 for rock since rock ties, and regret -2 for scissors since scissors lost
			
			so positive regret means we would rather have played that, negative means stay away

and then avg strat is just the normed strategy_sum, the average strategy across all iterations
	where each strategy is the normed all regrets sum
*/

enum struct action_t : uint8_t {
	ROCK = 0,
	PAPER = 1,
	SCISSORS = 2,
};

struct player_t {
	static constexpr int NUM_ACTIONS = 3;

	player_t() {
		m_regret_sum.fill(0);
		m_strategy.fill(0);
		m_strategy_sum.fill(0);
		m_action_utilities.fill(0);
	}
	std::vector<double>& get_strategy();

	uint8_t get_action(double rng) {
		double sum = 0;
		for (uint8_t i = 0; i < NUM_ACTIONS; i++) {
			sum += m_strategy[i];
			if (rng <= sum) {
				return i;
			}
		}
		return NUM_ACTIONS - 1;
	}

	void update_strategy(uint8_t hero, uint8_t villian) {
		// assumes self is hero
		m_action_utilities[villian] = 0;
		m_action_utilities[(villian + 1) % NUM_ACTIONS] = 1;
		m_action_utilities[((villian - 1) % NUM_ACTIONS + NUM_ACTIONS) % NUM_ACTIONS] = -1;

		for (size_t i = 0; i < NUM_ACTIONS; i++) {
			m_regret_sum[i] += m_action_utilities[i] - m_action_utilities[hero];
		}
		//m_action_utilities[(vill)]

		double norm = 0;
		for (size_t i = 0; i < NUM_ACTIONS; i++) {
			m_strategy[i] = std::max(m_regret_sum[i], 0.0);
			norm += m_strategy[i];
		}
		for (size_t i = 0; i < NUM_ACTIONS; i++) {
			if (norm > 0) {
				m_strategy[i] = m_strategy[i] / norm;
			}
			else {
				m_strategy[i] = 1.0 / NUM_ACTIONS;
			}
			m_strategy_sum[i] += m_strategy[i];
		}
	}

	std::vector<double> get_average_strategy() const {
		std::vector<double> avg_strategy(NUM_ACTIONS, 0);
		double norm = std::accumulate(m_strategy_sum.begin(), m_strategy_sum.end(), 0.0);
		
		for (size_t i = 0; i < NUM_ACTIONS; ++i) {
			if (norm > 0) {
				avg_strategy[i] = m_strategy_sum[i] / norm;
			}
			else {
				avg_strategy[i] = 1.0 / NUM_ACTIONS;
			}
		}

		return avg_strategy;
	}

	std::array<double, NUM_ACTIONS> m_regret_sum;
	std::array<double, NUM_ACTIONS> m_strategy;
	std::array<double, NUM_ACTIONS> m_strategy_sum;

	std::array<int, NUM_ACTIONS> m_action_utilities;
};

struct rps_t {
	static constexpr int NUM_ACTIONS = 3;
	rps_t(int iters)
		: m_iters(iters)
		, m_hero()
		, m_villian()
		, m_gen(m_rd())
		, m_rng(0, 1) {
	}

	double rng() {
		return m_rng(m_gen);
	}

	std::vector<double>& get_strategy() {
		double normalizing_sum = 0;
		for (size_t i = 0; i < NUM_ACTIONS; ++i) {
			m_strategy[i] = std::max(m_regret_sum[i], 0.0);
			normalizing_sum += m_strategy[i];
		}
		for (size_t i = 0; i < NUM_ACTIONS; ++i) {
			if (normalizing_sum > 0) {
				m_strategy[i] /= normalizing_sum;
			}
			else {
				m_strategy[i] = 1.0 / NUM_ACTIONS;
			}
			m_strategy_sum[i] += m_strategy[i];
		}
		return m_strategy;
	}



	uint8_t get_action(const std::vector<double>& strategy) {
		double r = rng();
		double cum_prob = 0;
		uint8_t i = 0;
		for (; i < NUM_ACTIONS - 1; ++i) {
			cum_prob += strategy[i];
			if (r < cum_prob) {
				// return static_cast<action_t>(i);
				return i;
			}
		}
		// return static_cast<action_t>(i);
		return i;
	}

	std::vector<double> get_average_strategy() {
		std::vector<double> avg_strategy(NUM_ACTIONS, 0);
		double normalizing_sum = 0;
		for (size_t i = 0; i < NUM_ACTIONS; ++i) {
			normalizing_sum += m_strategy_sum[i];
		}
		for (size_t i = 0; i < NUM_ACTIONS; ++i) {
			if (normalizing_sum > 0) {
				avg_strategy[i] = m_strategy_sum[i] / normalizing_sum;
			}
			else {
				avg_strategy[i] = 1.0 / NUM_ACTIONS;
			}
		}
		return avg_strategy;
	}

	void train() {
		std::vector<double> action_utility(NUM_ACTIONS, 0);
		for (int iter = 0; iter < m_iters; ++iter) {
			// get regret matched mixed strategy actions
			double r = rng();

			// auto& strategy = get_strategy();
			// auto h_action = get_action(strategy);
			// auto v_action = get_action(m_opponent_strategy);

			auto h_action = m_hero.get_action(r);
			r = rng();
			auto v_action = m_villian.get_action(r);

			m_hero.update_strategy(h_action, v_action);
			m_villian.update_strategy(v_action, h_action);
		}

		std::cout << "hero:\n";
		for (auto i : m_hero.get_average_strategy()) {
			std::cout << i << " ";
		}
		std::cout << "\nvillian:\n";
		for (auto i : m_villian.get_average_strategy()) {
			std::cout << i << " ";
		}
	}

	int m_iters;

	std::vector<double> m_regret_sum;
	std::vector<double> m_strategy;
	std::vector<double> m_strategy_sum;

	std::vector<double> m_opponent_strategy{ 0.4, 0.3, 0.3 };
	player_t m_hero;
	player_t m_villian;

	std::random_device m_rd;
	std::mt19937 m_gen;
	std::uniform_real_distribution<double> m_rng;
};



#endif
// TODO: Reference additional headers your program requires here.
