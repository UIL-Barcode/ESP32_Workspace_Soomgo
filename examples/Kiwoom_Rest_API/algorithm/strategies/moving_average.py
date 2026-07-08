from typing import Any

from algorithm.strategies.base import SignalResult, TradeEvent
from models.candle import Candle


class MovingAverageStrategy:
    name = "moving_average"

    def evaluate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> SignalResult:
        short_n = int(params["단기"])
        long_n = int(params["장기"])
        if len(candles) < long_n + 1:
            return SignalResult(
                signal="hold",
                reason=f"데이터 부족 (필요 {long_n + 1}일, 현재 {len(candles)}일)",
            )

        closes = [c.close for c in candles]
        short_now = _sma(closes, short_n)
        long_now = _sma(closes, long_n)
        short_prev = _sma(closes[:-1], short_n)
        long_prev = _sma(closes[:-1], long_n)

        indicators = {
            "단기MA": round(short_now, 2),
            "장기MA": round(long_now, 2),
            "종가": closes[-1],
        }

        if short_prev <= long_prev and short_now > long_now:
            return SignalResult(
                signal="buy",
                reason=f"골든크로스 (단기 {short_n} > 장기 {long_n})",
                indicators=indicators,
            )
        if short_prev >= long_prev and short_now < long_now:
            return SignalResult(
                signal="sell",
                reason=f"데드크로스 (단기 {short_n} < 장기 {long_n})",
                indicators=indicators,
            )
        return SignalResult(
            signal="hold",
            reason="크로스 없음",
            indicators=indicators,
        )

    def simulate(
        self,
        candles: list[Candle],
        params: dict[str, Any],
    ) -> tuple[list[TradeEvent], SignalResult]:
        long_n = int(params["장기"])
        trades: list[TradeEvent] = []
        final = SignalResult(signal="hold", reason="데이터 부족")

        for i in range(long_n + 1, len(candles) + 1):
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


def _sma(values: list[int], period: int) -> float:
    if len(values) < period:
        raise ValueError("SMA 기간 부족")
    window = values[-period:]
    return sum(window) / period
