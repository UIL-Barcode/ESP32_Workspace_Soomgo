from datetime import datetime
from pathlib import Path

from openpyxl import Workbook

from algorithm.strategies.base import SimulationResult


def save_simulation_excel(result: SimulationResult, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{result.stock_code}_{result.algorithm_id}_{result.start_date}_{result.end_date}_{stamp}.xlsx"
    path = output_dir / filename

    wb = Workbook()
    summary = wb.active
    summary.title = "요약"
    summary.append(["항목", "값"])
    rows = [
        ("종목코드", result.stock_code),
        ("알고리즘", result.algorithm_id),
        ("전략", result.strategy),
        ("시작일", result.start_date),
        ("종료일", result.end_date),
        ("봉 개수", result.candle_count),
        ("매매 횟수", len(result.trades)),
        ("최종 신호", result.final_signal.signal),
        ("최종 사유", result.final_signal.reason),
    ]
    for key, value in rows:
        summary.append([key, value])

    summary.append([])
    summary.append(["파라미터", "값"])
    for key, value in result.params.items():
        summary.append([key, value])

    if result.final_signal.indicators:
        summary.append([])
        summary.append(["지표", "값"])
        for key, value in result.final_signal.indicators.items():
            summary.append([key, value])

    trades = wb.create_sheet("매매")
    trades.append(["일자", "구분", "가격", "사유"])
    for trade in result.trades:
        trades.append([trade.date, trade.side, trade.price, trade.reason])

    wb.save(path)
    return path
