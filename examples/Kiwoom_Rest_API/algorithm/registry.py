from algorithm.strategies.base import Strategy
from algorithm.strategies.moving_average import MovingAverageStrategy
from algorithm.strategies.rsi import RsiStrategy

STRATEGIES: dict[str, Strategy] = {
    "moving_average": MovingAverageStrategy(),
    "rsi": RsiStrategy(),
}


def get_strategy(name: str) -> Strategy:
    strategy = STRATEGIES.get(name)
    if strategy is None:
        known = ", ".join(sorted(STRATEGIES))
        raise KeyError(f"알 수 없는 전략: {name} (가능: {known})")
    return strategy
