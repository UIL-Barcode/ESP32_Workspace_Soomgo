from dataclasses import dataclass


@dataclass
class OrderResult:
    side: str
    stock_code: str
    quantity: int
    price_type: str
    price: int | None
    order_no: str
    message: str
    practice_mode: bool
