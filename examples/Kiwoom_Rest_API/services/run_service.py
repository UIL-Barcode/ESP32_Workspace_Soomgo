from pathlib import Path

from algorithm.registry import get_strategy
from algorithm.strategies.base import SimulationResult, SignalResult, TradeEvent
from kiwoom.client import KiwoomClient
from services.algo_service import AlgoService
from services.sim_export import save_simulation_excel


class RunService:
    def __init__(
        self,
        kiwoom: KiwoomClient,
        algo_service: AlgoService,
        output_dir: Path,
    ) -> None:
        self._kiwoom = kiwoom
        self._algo = algo_service
        self._output_dir = output_dir

    def run_live(self, stock_code: str, algorithm_id: str) -> SignalResult:
        algo = self._algo.show_algorithm(algorithm_id)
        strategy = get_strategy(algo.strategy)
        params = {name: p.value for name, p in algo.params.items()}

        # 워밍업 여유를 두고 최근 일봉 조회
        from datetime import date

        today = date.today().strftime("%Y%m%d")
        candles = self._kiwoom.get_daily_chart(
            stock_code,
            today,
            max_pages=2,
        )
        if not candles:
            raise RuntimeError("일봉 데이터가 없습니다.")

        need = strategy.warmup_bars(params)
        recent = candles[-max(need, 30) :]
        return strategy.evaluate(recent, params)

    def run_simulation(
        self,
        stock_code: str,
        algorithm_id: str,
        start_date: str,
        end_date: str,
    ) -> SimulationResult:
        start = _normalize_ymd(start_date)
        end = _normalize_ymd(end_date)
        if start > end:
            raise ValueError("시작일이 종료일보다 늦습니다.")

        algo = self._algo.show_algorithm(algorithm_id)
        strategy = get_strategy(algo.strategy)
        params = {name: p.value for name, p in algo.params.items()}

        candles = self._kiwoom.get_daily_chart(
            stock_code,
            end,
            until_dt=start,
            max_pages=30,
        )
        filtered = [c for c in candles if start <= c.date <= end]
        if not filtered:
            raise RuntimeError(
                f"해당 구간에 일봉이 없습니다: {start}~{end} "
                f"(조회된 전체 {len(candles)}건)"
            )

        trades, final = strategy.simulate(filtered, params)
        total_return_pct, win_rate_pct, max_drawdown_pct, round_trip_count = (
            _compute_performance(trades)
        )
        result = SimulationResult(
            stock_code=stock_code,
            algorithm_id=algorithm_id,
            strategy=algo.strategy,
            start_date=start,
            end_date=end,
            params=params,
            candle_count=len(filtered),
            trades=trades,
            final_signal=final,
            total_return_pct=total_return_pct,
            win_rate_pct=win_rate_pct,
            max_drawdown_pct=max_drawdown_pct,
            round_trip_count=round_trip_count,
        )
        path = save_simulation_excel(result, self._output_dir)
        result.excel_path = str(path)
        return result


def _normalize_ymd(value: str) -> str:
    text = value.strip().replace("-", "").replace("/", "")
    if len(text) != 8 or not text.isdigit():
        raise ValueError(f"날짜 형식이 올바르지 않습니다 (YYYYMMDD): {value}")
    return text


def _compute_performance(trades: list[TradeEvent]) -> tuple[float, float, float, int]:
    """buy -> 다음 sell을 라운드트립으로 묶어 수익률/승률/MDD 계산.

    포지션 크기는 매 라운드트립마다 전액 재투입한다고 가정한 단순 복리 모델이다
    (자금 배분·수수료·슬리피지는 반영하지 않음 — 전략 비교용 참고 지표).
    """
    equity = 1.0
    peak = 1.0
    max_drawdown = 0.0
    wins = 0
    round_trips = 0
    entry_price: int | None = None

    for trade in trades:
        if trade.side == "buy":
            entry_price = trade.price
            continue
        if trade.side == "sell" and entry_price is not None:
            ret = (trade.price - entry_price) / entry_price
            trade.return_pct = round(ret * 100, 2)
            equity *= 1 + ret
            peak = max(peak, equity)
            max_drawdown = max(max_drawdown, (peak - equity) / peak)
            round_trips += 1
            if ret > 0:
                wins += 1
            entry_price = None

    total_return_pct = round((equity - 1) * 100, 2)
    win_rate_pct = round(wins / round_trips * 100, 2) if round_trips else 0.0
    max_drawdown_pct = round(max_drawdown * 100, 2)
    return total_return_pct, win_rate_pct, max_drawdown_pct, round_trips
