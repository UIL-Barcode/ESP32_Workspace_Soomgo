from typing import Any

from algorithm.strategies.base import SignalResult, TradeEvent
from models.candle import Candle


class RsiStrategy:
    name = "rsi"

    def evaluate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> SignalResult:
        period = int(params["기간"])
        oversold = int(params["과매도"])
        overbought = int(params["과매수"])
        if len(candles) < period + 1:
            return SignalResult(
                signal="hold",
                reason=f"데이터 부족 (필요 {period + 1}일, 현재 {len(candles)}일)",
            )

        rsi_now = _rsi([c.close for c in candles], period)
        rsi_prev = _rsi([c.close for c in candles[:-1]], period)
        indicators = {
            "RSI": round(rsi_now, 2),
            "과매도": oversold,
            "과매수": overbought,
            "종가": candles[-1].close,
        }

        if rsi_prev <= oversold < rsi_now:
            return SignalResult(
                signal="buy",
                reason=f"RSI 과매도 탈출 ({rsi_now:.1f} > {oversold})",
                indicators=indicators,
            )
        if rsi_prev >= overbought > rsi_now:
            return SignalResult(
                signal="sell",
                reason=f"RSI 과매수 이탈 ({rsi_now:.1f} < {overbought})",
                indicators=indicators,
            )
        return SignalResult(
            signal="hold",
            reason=f"RSI {rsi_now:.1f} (대기)",
            indicators=indicators,
        )

    def simulate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> tuple[list[TradeEvent], SignalResult]:
        period = int(params["기간"])
        trades: list[TradeEvent] = []
        final = SignalResult(signal="hold", reason="데이터 부족")

        for i in range(period + 2, len(candles) + 1):
            window = candles[:i]
            signal = self.evaluate(window, params)
            final = signal
            if signal.signal in ("buy", "sell"):
                trades.append(
                    TradeEvent(
                        date=window[-1].date,
                        side=signal.signal,
                        price=window[-1].close,
                        reason=signal.reason,
                    )
                )
        return trades, final


def _rsi(closes: list[int], period: int) -> float:
    if len(closes) < period + 1:
        raise ValueError("RSI 기간 부족")
    gains = 0.0
    losses = 0.0
    for i in range(-period, 0):
        diff = closes[i] - closes[i - 1]
        if diff >= 0:
            gains += diff
        else:
            losses -= diff
    avg_gain = gains / period
    avg_loss = losses / period
    if avg_loss == 0:
        return 100.0
    rs = avg_gain / avg_loss
    return 100.0 - (100.0 / (1.0 + rs))
