from dataclasses import dataclass, field
from typing import Any, Protocol

from models.candle import Candle


@dataclass
class SignalResult:
    signal: str  # buy / sell / hold
    reason: str = ""
    indicators: dict[str, Any] = field(default_factory=dict)


@dataclass
class TradeEvent:
    date: str
    side: str  # buy / sell
    price: int
    reason: str = ""


@dataclass
class SimulationResult:
    stock_code: str
    algorithm_id: str
    strategy: str
    start_date: str
    end_date: str
    params: dict[str, Any]
    candle_count: int
    trades: list[TradeEvent]
    final_signal: SignalResult
    excel_path: str = ""


class Strategy(Protocol):
    name: str

    def evaluate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> SignalResult:
        """최신 봉 기준 신호."""
        ...

    def simulate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> tuple[list[TradeEvent], SignalResult]:
        """전체 구간 시뮬. 반환: (트레이드, 최종 신호)."""
        ...
