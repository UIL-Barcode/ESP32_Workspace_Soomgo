from pathlib import Path

from algorithm.registry import get_strategy
from algorithm.strategies.base import SimulationResult, SignalResult
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

        need = _warmup_bars(params, algo.strategy)
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
        )
        path = save_simulation_excel(result, self._output_dir)
        result.excel_path = str(path)
        return result


def _normalize_ymd(value: str) -> str:
    text = value.strip().replace("-", "").replace("/", "")
    if len(text) != 8 or not text.isdigit():
        raise ValueError(f"날짜 형식이 올바르지 않습니다 (YYYYMMDD): {value}")
    return text


def _warmup_bars(params: dict, strategy_name: str) -> int:
    if strategy_name == "moving_average":
        return int(params.get("장기", 20)) + 5
    if strategy_name == "rsi":
        return int(params.get("기간", 14)) + 5
    return 40
