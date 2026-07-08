from dataclasses import dataclass


@dataclass(frozen=True)
class Candle:
    date: str  # YYYYMMDD
    open: int
    high: int
    low: int
    close: int
    volume: int
