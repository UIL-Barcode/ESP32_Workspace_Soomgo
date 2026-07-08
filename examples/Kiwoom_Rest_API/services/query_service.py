from typing import Any, Literal

from kiwoom.client import KiwoomClient
from kiwoom.format_util import normalize_stock_code, parse_amount, parse_rate
from models.query import HoldingItem, HoldingsSnapshot, StockQuote


class QueryService:
    def __init__(self, kiwoom: KiwoomClient, practice_mode: bool):
        self._kiwoom = kiwoom
        self._practice_mode = practice_mode

    def get_stock_price(self, stock_code: str) -> StockQuote:
        data = self._kiwoom.get_stock_price(stock_code)
        price_raw = data.get("cur_prc")
        if price_raw is None:
            raise RuntimeError(f"응답에 현재가가 없습니다: {data}")
        return self._parse_quote(stock_code, str(price_raw))

    def get_holdings(self, exchange: str = "KRX") -> HoldingsSnapshot:
        data = self._kiwoom.get_holdings(exchange)
        api_id = data.get("_api", "kt00005")
        if api_id == "kt00004":
            return self._parse_kt00004(data)
        return self._parse_kt00005(data)

    def _parse_quote(self, stock_code: str, price_raw: str) -> StockQuote:
        text = price_raw.strip()
        direction: Literal["up", "down", "flat"] = "flat"
        if text.startswith("-"):
            direction = "down"
            text = text[1:]
        elif text.startswith("+"):
            direction = "up"
            text = text[1:]
        return StockQuote(
            stock_code=stock_code,
            price=int(text),
            direction=direction,
            price_raw=price_raw,
        )

    def _parse_holding_item(self, item: dict[str, Any], qty_key: str) -> HoldingItem:
        code = normalize_stock_code(str(item.get("stk_cd", "")))
        name = str(item.get("stk_nm", "")).strip() or code
        profit_key = "evltv_prft" if "evltv_prft" in item else "pl_amt"
        return HoldingItem(
            stock_code=code,
            name=name,
            quantity=parse_amount(item.get(qty_key)),
            current_price=parse_amount(item.get("cur_prc")),
            eval_amount=parse_amount(item.get("evlt_amt")),
            profit_amount=parse_amount(item.get(profit_key)),
            profit_rate=parse_rate(item.get("pl_rt")),
        )

    def _parse_kt00005(self, data: dict[str, Any]) -> HoldingsSnapshot:
        items = [
            self._parse_holding_item(item, "cur_qty")
            for item in (data.get("stk_cntr_remn") or [])
        ]
        return HoldingsSnapshot(
            deposit=parse_amount(data.get("entr")),
            total_eval=parse_amount(data.get("evlt_amt_tot")),
            total_profit=parse_amount(data.get("tot_pl_tot")),
            total_profit_rate=parse_rate(data.get("tot_pl_rt")),
            items=items,
            practice_mode=self._practice_mode,
            api_id="kt00005",
        )

    def _parse_kt00004(self, data: dict[str, Any]) -> HoldingsSnapshot:
        items = [
            self._parse_holding_item(item, "rmnd_qty")
            for item in (data.get("stk_acnt_evlt_prst") or [])
        ]
        return HoldingsSnapshot(
            deposit=parse_amount(data.get("entr")),
            total_eval=parse_amount(data.get("aset_evlt_amt")),
            total_profit=parse_amount(data.get("lspft")),
            total_profit_rate=parse_rate(data.get("lspft_rt")),
            items=items,
            practice_mode=True,
            api_id="kt00004",
            total_purchase=parse_amount(data.get("tot_pur_amt")),
        )
