#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

namespace kuhn {

// lol
enum class action_t : uint8_t {
	PASS = 0,
	BET = 1,
	LAST = BET
};

constexpr size_t NUM_ACTIONS = 2;

template<typename T>
void normalize(T& arr) {
	double sum = 0;
	for (auto& s : arr) {
		sum += s;
	}

	for (auto& s : arr) {
		if (sum > 0) {
			s = s / sum;
		} else {
			s = 1.0 / arr.size();
		}
	}
}

using strategy_t = std::array<double, NUM_ACTIONS>;

template<typename stream_t>
stream_t& operator<<(stream_t& ss, const strategy_t& strat) {
	ss << std::fixed << std::setprecision(4);
	ss << "[PASS=" << strat[0] << " BET=" << strat[1] << "]";
	return ss;
}

struct node_t {

	node_t(std::string infoset) : m_infoset(infoset) {
		m_strategy.fill(0);
		m_regret_sum.fill(0);
		m_strategy_sum.fill(0);
	}

	const strategy_t& get_strategy(double realization_weight) {
		for (size_t i = 0; i < m_regret_sum.size(); i++) {
			m_strategy[i] = std::max(m_regret_sum[i], 0.0);
		}
		normalize(m_strategy);
		for (size_t i = 0; i < m_strategy_sum.size(); i++) {
			m_strategy_sum[i] += m_strategy[i] * realization_weight;
		}

		return m_strategy;
	}

	strategy_t get_average_strategy() const {
		strategy_t strat;
		std::copy(m_strategy_sum.begin(), m_strategy_sum.end(), strat.begin());
		normalize(strat);
		return strat;
	}

	std::string to_string() const {
		std::stringstream ss;
		ss << m_infoset << ": " << get_average_strategy();
		return ss.str();
	}
	
	const std::string m_infoset;
	strategy_t m_regret_sum;
	strategy_t m_strategy;
	strategy_t m_strategy_sum;
};

template<typename stream_t>
stream_t& operator<<(stream_t& ss, const node_t& node) {
	ss << node.to_string();
	return ss;
}

class solver_t {
	using DeckT = std::array<int, 3>;
public:
	void train(uint64_t iters) {
		std::random_device rd;
		std::mt19937 rng(rd());

		DeckT deck{0, 1, 2};
		double util = 0.0;
		for (size_t i = 0; i < iters; i++) {
			std::shuffle(deck.begin(), deck.end(), rng);
			std::string history;
			util += cfr(deck, history, 1, 1);
		}

		m_util = util / iters;
	}

	double cfr(const DeckT& cards, std::string& history, double p_hero, double p_villain) {
		size_t plays = history.size();
		size_t hero = plays % 2;
		size_t villain = 1 - hero;

		if (plays > 1) {
			bool hero_wins = cards[hero] > cards[villain];
			if (history.ends_with("bb")) return hero_wins ? 2 : -2;  // bet call
			if (history.ends_with("pp")) return hero_wins ? 1 : -1;  // check check
			if (history.ends_with("bp")) return 1;									 // bet fold
		}

		std::string infoset = std::to_string(cards[hero]) + history;
		auto nit = m_node_map.find(infoset);
		if (nit == m_node_map.end()) {
			nit = m_node_map.emplace(infoset, infoset).first;
		}
		auto& node = nit->second;

		// for each action, recursively call cfr with additional history and probability
		const auto& strategy = node.get_strategy(p_hero);
		strategy_t util;
		util.fill(0);
		double node_util = 0.0;
		for (size_t action = 0; action < NUM_ACTIONS; action++) {
			history.push_back(action == 0 ? 'p' : 'b');
			util[action] = -cfr(cards, history, p_villain, p_hero * strategy[action]);
			history.pop_back();
		}
		for (size_t action = 0; action < NUM_ACTIONS; action++) {
			node_util += strategy[action] * util[action];
		}

		// for each action, compute and accumulate counterfactual regret
		for (size_t action = 0; action < NUM_ACTIONS; action++) {
			double regret = util[action] - node_util;
			node.m_regret_sum[action] += p_villain * regret;
		}

		return node_util;
	}

	void show() {
		std::vector<std::string> nodes;
		for (const auto& [key, _] : m_node_map) {
			nodes.emplace_back(key);
		}
		std::sort(nodes.begin(), nodes.end(), [](const auto& s1, const auto& s2) {
			if (s1.size() == s2.size()) return s1 < s2;
			return s1.size() < s2.size();
		});

		std::cout << "Avg game value: " << std::to_string(m_util) << "\n";
		for (const auto& node : nodes) {
			std::cout << m_node_map.find(node)->second << "\n";
		}
	}

	std::unordered_map<std::string, node_t> m_node_map;
	double m_util;
};

}  // namespace kuhn
