from models.order import OrderResult
from models.query import HoldingsSnapshot, StockQuote
from models.algo import AlgorithmSpec, ParamChangeResult
from models.schedule import ScheduleSpec
from algorithm.strategies.base import SignalResult, SimulationResult


_SIGNAL_LABEL = {
    "buy": "매수",
    "sell": "매도",
    "hold": "대기",
}


def format_stock_quote(quote: StockQuote) -> str:
    sign_mark = {"up": " (상승 ▲)", "down": " (하락 ▼)", "flat": ""}[quote.direction]
    return (
        "========================================\n"
        f"종목코드 : {quote.stock_code}\n"
        f"현재가   : {quote.price:,}원{sign_mark}\n"
        "========================================"
    )


def format_holdings(snapshot: HoldingsSnapshot) -> str:
    lines = ["========================================", "[보유 종목]"]

    if snapshot.api_id == "kt00004":
        lines.extend([
            f"예수금   : {snapshot.deposit:,}원 (모의투자)",
            f"총평가   : {snapshot.total_eval:,}원",
            f"총매입   : {(snapshot.total_purchase or 0):,}원",
            f"손익     : {snapshot.total_profit:,}원 ({snapshot.total_profit_rate}%)",
        ])
    else:
        lines.extend([
            f"예수금   : {snapshot.deposit:,}원",
            f"총평가   : {snapshot.total_eval:,}원",
            f"총손익   : {snapshot.total_profit:,}원 ({snapshot.total_profit_rate}%)",
        ])

    lines.append("----------------------------------------")

    if not snapshot.items:
        lines.append("보유 종목 없음")
    else:
        for idx, item in enumerate(snapshot.items, 1):
            lines.append(f"{idx}. {item.name} ({item.stock_code})")
            lines.append(
                f"   수량 {item.quantity:,}주 | 현재가 {item.current_price:,}원"
            )
            lines.append(
                f"   평가 {item.eval_amount:,}원"
                f" | 손익 {item.profit_amount:,}원 ({item.profit_rate}%)"
            )

    lines.append("========================================")
    return "\n".join(lines)


def format_order_result(result: OrderResult) -> str:
    lines = [
        "========================================",
        f"{result.side} 주문 접수 완료",
        f"종목코드 : {result.stock_code}",
        f"수량     : {result.quantity}주",
        f"주문구분 : {result.price_type}",
    ]
    if result.price_type == "지정가" and result.price is not None:
        lines.append(f"가격     : {result.price:,}원")
    lines.append(f"주문번호 : {result.order_no}")
    if result.message:
        lines.append(f"메시지   : {result.message}")
    mode = "모의투자" if result.practice_mode else "실전투자"
    lines.append(f"모드     : {mode}")
    lines.append("========================================")
    return "\n".join(lines)


def format_algorithm_list(algorithms: list[AlgorithmSpec]) -> str:
    lines = ["========================================", "[알고리즘 목록]"]
    if not algorithms:
        lines.append("등록된 알고리즘이 없습니다.")
    else:
        for algo in algorithms:
            lines.append(f"• {algo.algorithm_id}")
            if algo.description:
                lines.append(f"  {algo.description}")
            lines.append(f"  전략: {algo.strategy}")
    lines.append("========================================")
    return "\n".join(lines)


def format_algorithm_detail(algo: AlgorithmSpec) -> str:
    lines = [
        "========================================",
        f"[{algo.algorithm_id}]",
        f"설명   : {algo.description}",
        f"전략   : {algo.strategy}",
        "----------------------------------------",
        "[파라미터]",
    ]
    if not algo.params:
        lines.append("(없음)")
    else:
        for name, param in algo.params.items():
            unit = f" {param.unit}" if param.unit else ""
            writable = "변경가능" if param.writable else "고정"
            lines.append(
                f"• {name}: {param.value}{unit} ({param.type}, {writable})"
            )
    lines.append("========================================")
    return "\n".join(lines)


def format_param_change(result: ParamChangeResult) -> str:
    unit = f" {result.unit}" if result.unit else ""
    return (
        "========================================\n"
        "파라미터 변경 완료\n"
        f"알고리즘 : {result.algorithm_id}\n"
        f"파라미터 : {result.param_name}\n"
        f"변경     : {result.old_value}{unit} → {result.new_value}{unit}\n"
        "========================================"
    )


def format_live_signal(stock_code: str, algorithm_id: str, signal: SignalResult) -> str:
    label = _SIGNAL_LABEL.get(signal.signal, signal.signal)
    lines = [
        "========================================",
        "[현재 신호]",
        f"종목코드 : {stock_code}",
        f"알고리즘 : {algorithm_id}",
        f"신호     : {label}",
        f"사유     : {signal.reason}",
    ]
    if signal.indicators:
        lines.append("----------------------------------------")
        for key, value in signal.indicators.items():
            lines.append(f"{key}: {value}")
    lines.append("========================================")
    return "\n".join(lines)


def _schedule_timing(spec: ScheduleSpec) -> str:
    if spec.type == "recurring":
        days = ",".join(spec.days or [])
        return f"매주 {days} {spec.time}"
    return f"1회 {spec.run_at}"


def format_schedule_list(schedules: list[ScheduleSpec]) -> str:
    lines = ["========================================", "[스케줄 목록]"]
    if not schedules:
        lines.append("등록된 스케줄이 없습니다.")
    else:
        for spec in schedules:
            state = "활성" if spec.enabled else "중지"
            lines.append(f"• {spec.id} [{state}] {spec.stock_code} · {spec.algorithm_id}")
            lines.append(f"  {_schedule_timing(spec)}")
            if spec.last_run:
                lines.append(f"  마지막 실행: {spec.last_run}")
    lines.append("========================================")
    return "\n".join(lines)


def format_schedule_detail(spec: ScheduleSpec, title: str) -> str:
    return (
        "========================================\n"
        f"{title}\n"
        f"스케줄ID : {spec.id}\n"
        f"종목코드 : {spec.stock_code}\n"
        f"알고리즘 : {spec.algorithm_id}\n"
        f"실행시점 : {_schedule_timing(spec)}\n"
        "========================================"
    )


def format_simulation_result(result: SimulationResult) -> str:
    label = _SIGNAL_LABEL.get(result.final_signal.signal, result.final_signal.signal)
    return (
        "========================================\n"
        "시뮬 완료 (엑셀 저장)\n"
        f"종목코드 : {result.stock_code}\n"
        f"알고리즘 : {result.algorithm_id}\n"
        f"구간     : {result.start_date} ~ {result.end_date}\n"
        f"봉 개수  : {result.candle_count}\n"
        f"매매 횟수: {len(result.trades)} (라운드트립 {result.round_trip_count})\n"
        f"누적수익률: {result.total_return_pct}% / 승률: {result.win_rate_pct}% / MDD: {result.max_drawdown_pct}%\n"
        f"최종 신호: {label}\n"
        f"파일     : {result.excel_path}\n"
        "========================================"
    )
