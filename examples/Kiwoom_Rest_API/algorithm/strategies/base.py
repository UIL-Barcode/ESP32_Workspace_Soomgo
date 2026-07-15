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
    return_pct: float | None = None  # sell 시점에만 설정 (직전 buy 대비 수익률)


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
    total_return_pct: float = 0.0
    win_rate_pct: float = 0.0
    max_drawdown_pct: float = 0.0
    round_trip_count: int = 0


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

    def warmup_bars(self, params: dict[str, Any]) -> int:
        """evaluate/simulate에 필요한 최소 선행 봉 수 (버퍼 포함)."""
        ...
