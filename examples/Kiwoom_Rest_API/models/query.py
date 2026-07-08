from dataclasses import dataclass, field
from typing import Literal


@dataclass
class StockQuote:
    stock_code: str
    price: int
    direction: Literal["up", "down", "flat"]
    price_raw: str


@dataclass
class HoldingItem:
    stock_code: str
    name: str
    quantity: int
    current_price: int
    eval_amount: int
    profit_amount: int
    profit_rate: str


@dataclass
class HoldingsSnapshot:
    deposit: int
    total_eval: int
    total_profit: int
    total_profit_rate: str
    items: list[HoldingItem] = field(default_factory=list)
    practice_mode: bool = False
    api_id: str = "kt00005"
    total_purchase: int | None = None
