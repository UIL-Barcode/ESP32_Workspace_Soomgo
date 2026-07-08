from config.settings import Config
from kiwoom.client import KiwoomClient
from models.order import OrderResult
from notify.notifier import Notifier


class OrderService:
    def __init__(self, kiwoom: KiwoomClient, notifier: Notifier, config: Config):
        self._kiwoom = kiwoom
        self._notifier = notifier
        self._config = config

    def buy(
        self,
        stock_code: str,
        quantity: int,
        price: int | None = None,
        exchange: str = "KRX",
    ) -> OrderResult:
        data = self._kiwoom.buy_order(stock_code, quantity, price, exchange)
        return self._to_result("매수", data)

    def sell(
        self,
        stock_code: str,
        quantity: int,
        price: int | None = None,
        exchange: str = "KRX",
    ) -> OrderResult:
        data = self._kiwoom.sell_order(stock_code, quantity, price, exchange)
        return self._to_result("매도", data)

    def _to_result(self, side: str, data: dict) -> OrderResult:
        order_info = data.get("_order", {})
        trde_tp = order_info.get("trde_tp", "")
        price_type = "시장가" if trde_tp == "3" else "지정가"
        ord_uv = order_info.get("ord_uv", "")
        price = int(ord_uv) if ord_uv else None

        result = OrderResult(
            side=side,
            stock_code=order_info.get("stk_cd", ""),
            quantity=int(order_info.get("ord_qty", 0)),
            price_type=price_type,
            price=price,
            order_no=str(data.get("ord_no", "")),
            message=str(data.get("return_msg", "")).strip(),
            practice_mode=self._config.practice_mode,
        )
        self._notifier.info(
            f"{side} 주문 접수: {result.stock_code} {result.quantity}주 "
            f"(주문번호 {result.order_no})"
        )
        return result
