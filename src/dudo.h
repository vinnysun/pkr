#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "throw.h"


namespace dudo {

// 1v1 dudo
// infoset consists of your die num, and claim history

// sucks but just do it for now
constexpr size_t NUM_SIDES = 3;
constexpr size_t NUM_ACTIONS = (2 /*dice*/ * NUM_SIDES) + 1;  // dudo
constexpr size_t DUDO = NUM_ACTIONS - 1;
//constexpr std::array<int, NUM_ACTIONS> g_claim_num = {1,1,1,1,1,1,2,2,2,2,2,2};
//constexpr std::array<int, NUM_ACTIONS> g_claim_rank = {2,3,4,5,6,1,2,3,4,5,6,1};

struct claim_t {
	uint8_t m_num;
	uint8_t m_rank;
};

// constexpr claim_t DUDO = {0, 0};

constexpr std::array<claim_t, NUM_ACTIONS> g_claims = []{
	std::array<claim_t, NUM_ACTIONS> claims;
	for (size_t i = 0; i < claims.size() - 1; i++) {
		uint8_t side = static_cast<uint8_t>((i / NUM_SIDES) + 1);
		uint8_t rank = static_cast<uint8_t>(((i + 1) % NUM_SIDES) + 1);
		claims[i] = claim_t {side, rank};
	}
	claims.back() = claim_t {0, 0};
	return claims;
}();

struct strategy_t : std::array<double, NUM_ACTIONS> {
	/*
	template<typename... Args>
	strategy_t(Args&&... args)
	: std::array<double, NUM_ACTIONS>(std::forward<Args>(args)...) {
		fill(0.0);
	}
	*/
	strategy_t() : std::array<double, NUM_ACTIONS>() {
		fill(0.0);
	}

	void normalize() {
		double sum = 0;
		for (auto& s : *this) {
			sum += s;
		}

		for (auto& s : *this) {
			if (sum > 0) {
				s = s / sum;
			} else {
				s = 1.0 / size();
			}
		}
	}
};

std::ostream& operator<<(std::ostream& ss, const strategy_t& strat) {
	ss << std::fixed << std::setprecision(4);
	ss << '[';
	for (size_t i = 0; i < strat.size(); i++) {
		ss << strat[i];
		if (i != strat.size() - 1) ss << ' ';
	}
	ss << ']';
	return ss;
}

struct history_t {

	std::string to_hr_string() const {
		std::string s;
		bool first = true;
		for (size_t i = 0; i < size() - 1; i++) {
			if (test(i)) {
				if (not first) s.push_back(',');
				first = false;
				s.push_back('0' + g_claims[i].m_num);
				s.push_back('*');
				s.push_back('0' + g_claims[i].m_rank);
			}
		}
		
		if (test(DUDO)) {
			if (not first) s.push_back(',');
			s.append("dudo");
		}
		return s;
	}

	constexpr size_t size() const {
		return NUM_ACTIONS;
	}

	void set(size_t action) {
		assert(not test(action));
		// m_history.set(action);
		m_history |= (1 << action);
		//m_last_played = action;
	}

	void unset(size_t action) {
		assert(test(action));
		// m_history.set(action, false);
		m_history &= ~(1 << action);
	}

	bool test(size_t i) const {
		// return m_history.test(i);
		return m_history & (1 << i);
	}

	size_t last_played() const {
		for (size_t i = size() - 1; i < size(); i--) {
			if (test(i)) return i;
		}
		assert(false);
		return NUM_ACTIONS;
	}

	size_t next_possible() const {
		if (ended()) return size();
		if (m_history == 0) return 0;
		return last_played() + 1;
	}

	bool ended() const {
		return test(DUDO);
	}

	void reset() {
		m_history = 0;
	}

	bool operator==(const history_t& other) const {
		return m_history == other.m_history;
	}

	//size_t m_last_played = NUM_ACTIONS;
	//std::bitset<NUM_ACTIONS> m_history;
	uint16_t m_history = 0;
};

template<typename stream_t>
stream_t& operator<<(stream_t& ss, const history_t& history) {
	for (size_t i = 0; i < history.size(); i++) {
		ss << bool(history.m_history & (1 << i));
	}
	ss << " " << history.to_hr_string();
	return ss;
}

struct infoset_t {
	infoset_t(int roll, history_t history) : m_roll(roll), m_history(history) {}

	void inc() const {
		m_num++;
	}

	std::string to_hr_string() const {
		return std::to_string(m_roll) + "-" + m_history.to_hr_string();
	}

	bool operator==(const infoset_t& other) const {
		return m_roll == other.m_roll and m_history == other.m_history;
	}

	bool operator<(const infoset_t& other) const {
		if (m_roll != other.m_roll) return m_roll < other.m_roll;
		for (size_t i = 0; i < m_history.size(); i++) {
			if (m_history.test(i) != other.m_history.test(i)) return m_history.test(i) > other.m_history.test(i);
		}
		return false;
	}

	//const int m_roll;
	//const history_t m_history;
	int m_roll;
	history_t m_history;
	mutable size_t m_num = 0;
};

template<typename stream_t>
stream_t& operator<<(stream_t& ss, const infoset_t infoset) {
	ss << infoset.m_roll << " (" << infoset.m_num << "): " << infoset.m_history;
	return ss;
}

struct infoset_hash {
	size_t operator()(const dudo::infoset_t& infoset) const {
		size_t n = (infoset.m_roll << NUM_ACTIONS) | infoset.m_history.m_history;
		return std::hash<size_t>()(n);
	}
};

struct node_t {
	node_t(const infoset_t infoset) : m_infoset(infoset) {}

	const strategy_t& get_strategy(double weight) {
		for (size_t i = 0; i < m_regret_sum.size(); i++) {
			m_strategy[i] = std::max(m_regret_sum[i], 0.0);
		}
		m_strategy.normalize();
		for (size_t i = 0; i < m_strategy_sum.size(); i++) {
			m_strategy_sum[i] += m_strategy[i] * weight;
		}

		return m_strategy;
	}

	strategy_t get_average_strategy() const {
		strategy_t strat;
		std::copy(m_strategy_sum.begin(), m_strategy_sum.end(), strat.begin());
		strat.normalize();
		return strat;
	}

	std::string to_hr_string() const {
		std::stringstream ss;
		ss << m_infoset << ": " << get_average_strategy();
		return ss.str();
	}

	template<typename stream_t>
	stream_t& operator<<(stream_t& ss) {
		ss << m_infoset << ": " << get_average_strategy();
		return ss;
	}

	const infoset_t m_infoset;
	strategy_t m_regret_sum;
	strategy_t m_strategy;
	strategy_t m_strategy_sum;
};

template<typename stream_t>
stream_t& operator<<(stream_t& ss, const node_t& node) {
	ss << node.to_hr_string();
	return ss;
}

class solver_t {
public:
	void train(uint64_t iters) {
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<> die(1, NUM_SIDES);

		double util = 0.0;
		for (size_t i = 0; i < iters; i++) {
			int p1 = die(rng);
			int p2 = die(rng);

			history_t history;
			util += cfr(p1, p2, history, 1, 1);
		}

		m_avg_util = util / iters;
	}

	double cfr(int hero, int villain, history_t& history, double p_hero, double p_villain) {
		auto iit = m_infosets.find({hero, history});
		if (iit == m_infosets.end()) {
			iit = m_infosets.emplace_hint(iit, hero, history);
		}
		auto& infoset = *iit;

		auto nit = m_node_map.find(infoset);
		if (nit == m_node_map.end()) {
			nit = m_node_map.emplace(infoset, infoset).first;
		}
		auto& node = nit->second;
		node.m_infoset.inc();

		auto calc_total = [&hero, &villain](auto called) {
			auto count = (hero == called) + (villain == called);
			if (called != 1) count += (hero == 1) + (villain == 1);
			return count;
		};
		//std::cout << history.to_hr_string() << "\n";
		if (history.ended()) {
			// dudo called, villain challenges our claim
			const claim_t& claim = g_claims.at(history.last_played());
			int count = calc_total(claim.m_rank);
			// TODO see what happens if this returns the num die we would have won
			if (count > claim.m_num) return 1;
			if (count < claim.m_num) return -1;
			if (count == claim.m_num) return -1;  // "everyone loses 1 die" == we lose
		}

		const auto& strategy = node.get_strategy(p_hero);
		strategy_t util;
		double node_util = 0.0;
		for (size_t i = history.next_possible(); i < history.size(); i++) {
			history.set(i);
			util[i] += -cfr(villain, hero, history, p_villain, p_hero * strategy[i]);
			history.unset(i);
		}
		for (size_t i = 0; i < history.size(); i++) {
			node_util += strategy[i] * util[i];
		}

		for (size_t i = 0; i < history.size(); i++) {
			double regret = util[i] - node_util;
			node.m_regret_sum[i] += p_villain * regret;
		}

		return node_util;
	}

	void show() {
		/*
		std::vector<infoset_t> nodes;
		for (const auto& [key, _] : m_node_map) {
			nodes.emplace_back(key);
		}
		std::sort(nodes.begin(), nodes.end(), [](const auto& a, const auto& b) {
			if (a.m_roll != b.m_roll) {
				return a.m_roll < b.m_roll;
			}
			for (size_t i = 0; i < a.m_history.size(); i++) {
				if (a.m_history.test(i) != b.m_history.test(i)) {
					return static_cast<int>(a.m_history.test(i)) < static_cast<int>(b.m_history.test(i));
				}
			}
			return false;
		});
		*/
		std::set<int> rolls;
		for (const auto& infoset : m_infosets) {
			rolls.emplace(infoset.m_roll);
			std::cout << m_node_map.find(infoset)->second << "\n";
		}

		std::cout << "Avg game value: " << std::to_string(m_avg_util) << "\n";
		std::cout << "Num nodes: " << m_node_map.size() << "\n";
		/*
		for (int r : rolls) {
			std::cout << r << ",";
		}
		*/
		std::cout << "\n";
		for (const auto& c : g_claims) {
			std::cout << "(" << static_cast<int>(c.m_num) << "*" << static_cast<int>(c.m_rank) << ")\n";
		}
	}

	std::set<infoset_t> m_infosets;
	std::unordered_map<infoset_t, node_t, infoset_hash> m_node_map;
	double m_avg_util = 0;
};

}  // dudo

