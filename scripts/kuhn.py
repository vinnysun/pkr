import numpy as np
import random
from enum import Enum
import time
import argparse

# basically copied from cfr.pdf for now

# JQK instead of 012
# class Card(Enum):
#     J = 0
#     Q = 1
#     K = 2

# class Action(Enum):
#     PASS = 0
#     BET = 1
PASS = 0
BET = 1
NUM_ACTIONS = 2

# string (infoset) -> node
node_map = {}

def normalize(arr):
    norm = np.sum(arr)
    if norm > 0:
        return arr / norm
    else:
        return np.ones_like(arr) / len(arr)

class Node:
    def __init__(self, infoset: str):
        self.infoset = infoset
        self.regret_sum = np.zeros(NUM_ACTIONS)
        self.strategy = np.zeros(NUM_ACTIONS)
        self.strategy_sum = np.zeros(NUM_ACTIONS)

    def get_strategy(self, realization_weight: float) -> np.array:
        # get current information set mixed strategy through regret matching
        self.strategy = normalize(np.clip(self.regret_sum, a_min=0, a_max=np.inf))
        self.strategy_sum += self.strategy * realization_weight
        return self.strategy

    def get_average_strategy(self) -> np.array:
        return normalize(self.strategy_sum)

    def __str__(self):
        avg_strat = self.get_average_strategy()
        return f'{self.infoset}: {str(np.round(avg_strat, 2))}'

def cfr(cards: np.array, history: str, p0: float, p1: float) -> float:
    global node_map
    plays = len(history)
    player = plays % 2
    opponent = 1 - player

    # return payoff for terminal states - p is check or fold, b is bet or call
    # handles x/x, x/b/c, x/b/f, b/c, b/f
    if plays > 1:
        terminal_pass = history[plays - 1] == 'p'  # last action was pass?
        double_bet = history[plays - 2:plays] == 'bb'  # bet/call?
        player_card_higher = cards[player] > cards[opponent]
        if terminal_pass:
            if history == 'pp':
                return 1 if player_card_higher else -1
            else:
                return 1
        elif double_bet:
            return 2 if player_card_higher else -2

    infoset = str(cards[player]) + history

    # get infoset node or create if nonexistant
    if infoset not in node_map:
        node_map[infoset] = Node(infoset)
    node = node_map[infoset]

    # for each action, recursively call cfr with additional history and probability
    strategy = node.get_strategy(p0 if player == 0 else p1)
    util = np.zeros_like(strategy)
    node_util = 0.0
    for a in range(NUM_ACTIONS):
        next_history = history + ('p' if a == 0 else 'b')
        if player == 0:
            util[a] = -cfr(cards, next_history, p0 * strategy[a], p1)
        else:
            util[a] = -cfr(cards, next_history, p0, p1 * strategy[a])
    node_util = np.sum(strategy * util)

    # for each action, compute and accumulate counterfactual regret
    for a in range(NUM_ACTIONS):
        regret = util[a] - node_util
        if player == 0:
            node.regret_sum[a] += p1 * regret
        else:
            node.regret_sum[a] += p0 * regret

    return node_util

def train(iters: int) -> None:
    start = time.time()
    cards = [0, 1, 2]
    util = 0.0
    for _ in range(iters):
        random.shuffle(cards)
        util += cfr(cards, '', 1, 1)
    end = time.time()

    print(f'Elapsed: {end - start:.2f} seconds')
    print(f'Avg game value: {util / iters}')
    nodes = list(node_map.values())
    nodes = sorted(nodes, key=lambda x: (len(x.infoset), x.infoset))
    for node in nodes:
        print(node)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--iters', required=True, type=int)
    args = parser.parse_args()

    train(args.iters)


