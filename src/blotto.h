#ifndef bobafett_h__
#define bobafett_h__

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <array>
#include <numeric>

// https://arxiv.org/pdf/1811.00164
// http://modelai.gettysburg.edu/2013/cfr/cfr.pdf - paper with example games
// https://github.com/ArmanMielke/simple-poker-cfr
// https://poker.cs.ualberta.ca/publications/NIPS07-cfr.pdf - og paper
// https://rnikhil.com/2023/12/31/ai-cfr-solver-poker.html - blog post
// https://aipokertutorial.com/the-cfr-algorithm/ - url name self explanatory
// https://github.com/tansey/pycfr
// https://www.youtube.com/watch?v=ygDt_AumPr0 - step by step 1 iter of algo

namespace bf {

template<typename T>
constexpr T n_choose_k(T n, T k) {
	if (k > n) return 0;
	if (k * 2 > n) k = n-k;
	if (k == 0 or k == n) return 1;

	T result = n;
	for (T i = 2; i <= k; ++i) {
			result *= (n - i + 1);
			result /= i;
	}
	return result;
}

template<typename T>
int battle(const T& h, const T& v) {
	int score = 0;
	assert(h.size() == v.size());
	for (size_t i = 0; i < h.size(); i++) {
		if (h[i] > v[i]) score++;
		else if (h[i] < v[i]) score--;
	}
	return score > 0 ? 1 : (score < 0 ? -1 : 0);
}

template <size_t N>
struct strategy_t : public std::array<uint64_t, N> {
	template<typename... Args>
	strategy_t(Args&&... args)
		// : std::array<uint64_t, N>(std::forward<Args>(args)...)
		: std::array<uint64_t, N>()//std::forward<Args>(args)...)
	{
		std::cout << "strat constructor\n";
		// static_assert(sizeof...(args) == N, "bad num args");
		size_t idx = 0;
		for (auto n : {args...}) {
			std::cout << n;
			this[idx++] = n;
		}
	}

	int battle(const strategy_t<N>& other) const {
		int score = 0;
		for (int i = 0; i < this->size(); i++) {
			//std::cout << this->at(i) << " " << other.at(i) << " ";
			if (this->at(i) > other.at(i)) {
				score++;
			}
			else if (this->at(i) < other.at(i)) {
				score--;
			}
			//std::cout << score << "\n";
		}
		
		return score > 0 ? 1 : (score < 0 ? -1 : 0);
	}
};

template <typename stream_t, size_t N>
stream_t& operator<<(stream_t& ss, const strategy_t<N>& strat) {
	for (auto s : strat) {
		ss << s << " ";
	}
	ss << "\n";
	return ss;
}

/*
template<size_t S, size_t N>
struct action_t {
	template<typename... Args>
	action_t(Args&&... args) : m_battles(std::forward<Args>(args)...) {
		static_assert(sizeof...(args) == N, "bad num of args");
		static_assert((... + args) == S, "bad sum of soldiers");
	}

	// may need cmp func

	std::array<uint8_t, N> m_battles;
};
*/

template <size_t S, size_t N>
struct player_t {
	player_t(const std::vector<std::vector<uint64_t>>& all_actions)
		: m_all_actions(all_actions)
		, m_utilities(m_all_actions.size(), 0)
		, m_regret_sum(m_all_actions.size(), 0)
		, m_strategy(m_all_actions.size(), 0)
		, m_strategy_sum(m_all_actions.size(), 0)
	{

	}

	/*
	void init(const std::all_actions) {
		m_all_actions = all_actions;
	}
	*/

	uint64_t get_action(double rng) {
		double sum = 0;
		for (uint64_t i = 0; i < m_strategy.size(); i++) {
			sum += m_strategy[i];
			// std::cout << m_strategy[i] << " " << sum << " ";
			if (rng <= sum) {
				// std::cout << "\n";
				return i;
			}
		}
		// std::cout << "\n";
		return m_strategy.size() - 1;
	}

	std::vector<double> average_strategy() const {
		std::vector<double> avg_strat;
		double norm = std::accumulate(m_strategy_sum.begin(), m_strategy_sum.end(), 0.0);
		for (double p : m_strategy_sum) {
			avg_strat.push_back(p / norm);
		}
		return avg_strat;
	}

	uint64_t get_avg_action(double rng) {
		double sum = 0;
		auto strat = average_strategy();
		for (uint64_t i = 0; i < strat.size(); i++) {
			sum += strat[i];
			// std::cout << m_strategy[i] << " " << sum << " ";
			if (rng <= sum) {
				// std::cout << "\n";
				return i;
			}
		}
		// std::cout << "\n";
		return strat.size() - 1;
	}

	void update_strategy(size_t hero_action, size_t villian_action) {
		// TODO precompute result of every possible pair of actions, use that to populate this
		// this vector is "if they play villian_action, what's my utility for each action i could've taken?"
		for (size_t i = 0; i < m_utilities.size(); i++) {
			// const auto& hero = m_all_actions[i];
			// const auto& villian = m_all_actions[villian_action];
			// m_utilities[i] = hero.battle(villian);
			m_utilities[i] = battle(m_all_actions[i], m_all_actions[villian_action]);
		}

		// now that we have our utility of our possible actions,
		// calc how much we regret the one that we played
		for (size_t i = 0; i < m_regret_sum.size(); i++) {
			m_regret_sum[i] += m_utilities[i] - m_utilities[hero_action];
		}

		// std::cout << "m_strategy: ";
		for (size_t i = 0; i < m_strategy.size(); i++) {
			m_strategy[i] = std::max(m_regret_sum[i], 0.0);
			// std::cout << m_strategy[i] << " ";
		}
		// std::cout << "\n";

		double norm = std::accumulate(m_strategy.begin(), m_strategy.end(), 0.0);
		for (size_t i = 0; i < m_strategy.size(); i++) {
			if (norm > 0) {
				m_strategy[i] = m_strategy[i] / norm;
			}
			else {
				m_strategy[i] = 1.0 / norm;
			}
		}
		
		for (size_t i = 0; i < m_strategy_sum.size(); i++) {
			m_strategy_sum[i] += m_strategy[i];
		}
	}

	// const std::vector<strategy_t<N>>& m_all_actions;
	// const std::array<strategy_t<N>, n_choose_k(S+2, N+1)>& m_all_actions;
	const std::vector<std::vector<uint64_t>>& m_all_actions;
	std::vector<double> m_utilities;
	std::vector<double> m_regret_sum;
	std::vector<double> m_strategy;
	std::vector<double> m_strategy_sum;
};

template<size_t S, size_t N>
struct blotto_t {
	static constexpr uint64_t NUM_ALL_STRATEGIES = n_choose_k(S + 2, N + 1);
	blotto_t()
		: m_all_strategies(generate_all_strategies(S, N))
		, m_hero(m_all_strategies)
		, m_villian(m_all_strategies)
		, m_dist(std::uniform_real_distribution<>(0, 1))
		, m_rng(m_rd())
	{
		/*
		m_all_strategies = generate_all_strategies(S, N);
		m_hero.init(m_all_strategies);
		m_villian.init(m_all_strategies);
		*/
		std::cout << "num strategies: " << m_all_strategies.size() << "\n";
		for (const auto& strat : m_all_strategies) {
			for (auto n : strat) {
				std::cout << n;
			}
			std::cout << "\n";
		}
	}

	auto generate_all_strategies(size_t num_soldiers, size_t num_battlefields) {
		std::vector<std::vector<uint64_t>> all_strategies;
		// all_strategies.reserve(NUM_ALL_STRATEGIES);
		// TODO need a version not harded coded for 3 battlefields
		for (uint64_t b1 = 0; b1 <= S; b1++) {
			for (uint64_t b2 = 0; b2 <= S; b2++) {
				for (uint64_t b3 = 0; b3 <= S; b3++) {
					if (b1 + b2 + b3 == S) {
						all_strategies.push_back(std::vector{b1, b2, b3});
						// all_strategies.emplace_back({b1, b2, b3});
					}
				}
			}
		}

		return all_strategies;
	}

	double rng() {
		return m_dist(m_rng);
	}

	void train(uint64_t iters) {
		for (size_t i = 0; i < iters; i++) {
			double r = rng();
			// std::cout << r << "\n";
			auto hero = m_hero.get_action(r);
			r = rng();
			// std::cout << r << "\n";
			auto villian = m_villian.get_action(r);

			/*
			std::cout << "iter " << i << ":\n";
			std::cout << "hero play (" << hero << "): ";
			for (auto s : m_all_strategies[hero]) {
				std::cout << s << " ";
			}
			std::cout << "\n";
			std::cout << "villian play (" << villian << "): ";
			for (auto s : m_all_strategies[villian]) {
				std::cout << s << " ";
			}
			std::cout << "\n";
			*/

			m_hero.update_strategy(hero, villian);
			m_villian.update_strategy(villian, hero);

			// std::cout << "----------------------\n";
			

		}
		auto hero = m_hero.average_strategy();
		auto villian = m_villian.average_strategy();
		
		std::cout << "hero:\n";
		for (int i = 0; i < hero.size(); i++) {
			const auto& strat = m_all_strategies[i];
			double p = hero[i];
			std::cout << "[" << strat[0] << " " << strat[1] << " " << strat[2] << "] "
				<< std::round(p * 1000.0) / 1000.0 << "\n";
		}
		
		std::cout << "\nvillian:\n";
		for (int i = 0; i < villian.size(); i++) {
			const auto& strat = m_all_strategies[i];
			double p = villian[i];
			std::cout << "[" << strat[0] << " " << strat[1] << " " << strat[2] << "] "
				<< std::round(p * 1000.0) / 1000.0 << "\n";
		}

		int result = 0;
		for (int i = 0; i < 500000; i++) {
			double r = rng();
			// const auto& hero_action = m_all_strategies[m_hero.get_avg_action(r)];
			size_t hero_action = m_hero.get_action(r);
			r = rng();
			// const auto& villian_action = m_all_strategies[m_villian.get_avg_action(r)];
			size_t villian_action = m_villian.get_action(r);
			//std::cout << "hero action: " << hero_action << "villian action: " 
			//	<< villian_action << "score: " << hero_action.battle(villian_action) << "\n";

			// result += hero_action.battle(villian_action);
			result += battle(m_all_strategies[hero_action], m_all_strategies[villian_action]);
		}

		std::cout << "result: " << result << "\n";
	}

	// constexpr static uint64_t NUM_ALL_STRATEGIES = N ** S;
	// with N battlefields and S soldiers, (S+2 choose N-1) ways to alloc
	// put S soldiers in a line, place N - 1 "dividers"
	//   S S S S S
	//  |     |
	// this alloc is 0/3/2
	// const std::array<strategy_t<N>, NUM_ALL_STRATEGIES> m_all_strategies;
	// const std::vector<strategy_t<N>> m_all_strategies;
	const std::vector<std::vector<uint64_t>> m_all_strategies;

	player_t<S, N> m_hero;
	player_t<S, N> m_villian;

	std::uniform_real_distribution<double> m_dist;
	std::random_device m_rd;
	std::mt19937 m_rng;
};

}  // namespace blotto

#endif
