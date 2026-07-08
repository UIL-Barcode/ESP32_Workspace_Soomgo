from typing import Any, Optional

import requests

from config.settings import Config
from kiwoom import account, auth, chart, order, stock, stock_list
from models.candle import Candle
from notify.notifier import Notifier


class KiwoomClient:
    def __init__(self, config: Config, notifier: Notifier):
        self.config = config
        self.notifier = notifier
        self.session = requests.Session()
        self.token: Optional[str] = None

    def authenticate(self) -> bool:
        token = auth.fetch_token(self.session, self.config)
        if not token:
            self.notifier.error("토큰 발급 실패")
            return False
        self.token = token
        self.notifier.info("접근 토큰 발급 성공")
        return True

    def get_stock_price(self, stock_code: str) -> dict[str, Any]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        return stock.fetch_price(self.session, self.config, self.token, stock_code)

    def get_stock_list(self) -> list[dict[str, Any]]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        return stock_list.fetch_stock_list(self.session, self.config, self.token)

    def get_daily_chart(
        self,
        stock_code: str,
        base_dt: str,
        *,
        until_dt: str | None = None,
        max_pages: int = 20,
    ) -> list[Candle]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        return chart.fetch_daily_chart(
            self.session,
            self.config,
            self.token,
            stock_code,
            base_dt,
            until_dt=until_dt,
            max_pages=max_pages,
        )

    def get_holdings(self, exchange: str = "KRX") -> dict[str, Any]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        if self.config.practice_mode:
            return account.fetch_holdings_practice(
                self.session, self.config, self.token, exchange
            )
        return account.fetch_holdings_live(
            self.session, self.config, self.token, exchange
        )

    def buy_order(
        self,
        stock_code: str,
        quantity: int,
        price: int | None = None,
        exchange: str = "KRX",
    ) -> dict[str, Any]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        return order.buy_stock(
            self.session,
            self.config,
            self.token,
            stock_code,
            quantity,
            price,
            exchange,
        )

    def sell_order(
        self,
        stock_code: str,
        quantity: int,
        price: int | None = None,
        exchange: str = "KRX",
    ) -> dict[str, Any]:
        if not self.token:
            raise RuntimeError("토큰이 없습니다. authenticate()를 먼저 호출하세요.")
        return order.sell_stock(
            self.session,
            self.config,
            self.token,
            stock_code,
            quantity,
            price,
            exchange,
        )
