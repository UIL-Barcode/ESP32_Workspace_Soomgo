from typing import Any

import requests

from config.settings import Config


def _post_order(
    session: requests.Session,
    config: Config,
    token: str,
    api_id: str,
    payload: dict[str, Any],
) -> dict[str, Any]:
    url = f"{config.base_url}/api/dostk/ordr"
    headers = {
        "Content-Type": "application/json;charset=UTF-8",
        "authorization": f"Bearer {token}",
        "api-id": api_id,
    }
    response = session.post(url, headers=headers, json=payload, timeout=10)
    response.raise_for_status()
    data = response.json()
    if data.get("return_code", 0) != 0:
        raise RuntimeError(data.get("return_msg", "주문 실패"))
    return data


def _build_payload(
    stock_code: str,
    quantity: int,
    price: int | None,
    exchange: str = "KRX",
) -> dict[str, Any]:
    if quantity <= 0:
        raise ValueError("수량은 1 이상이어야 합니다.")

    if price is not None and price <= 0:
        raise ValueError("가격은 0보다 커야 합니다.")

    # trde_tp: 0=지정가, 3=시장가
    if price is None:
        trde_tp = "3"
        ord_uv = ""
    else:
        trde_tp = "0"
        ord_uv = str(price)

    return {
        "dmst_stex_tp": exchange,
        "stk_cd": stock_code,
        "ord_qty": str(quantity),
        "ord_uv": ord_uv,
        "trde_tp": trde_tp,
        "cond_uv": "",
    }


def buy_stock(
    session: requests.Session,
    config: Config,
    token: str,
    stock_code: str,
    quantity: int,
    price: int | None = None,
    exchange: str = "KRX",
) -> dict[str, Any]:
    """주식 매수주문 (kt10000)"""
    payload = _build_payload(stock_code, quantity, price, exchange)
    data = _post_order(session, config, token, "kt10000", payload)
    data["_side"] = "매수"
    data["_order"] = payload
    return data


def sell_stock(
    session: requests.Session,
    config: Config,
    token: str,
    stock_code: str,
    quantity: int,
    price: int | None = None,
    exchange: str = "KRX",
) -> dict[str, Any]:
    """주식 매도주문 (kt10001)"""
    payload = _build_payload(stock_code, quantity, price, exchange)
    data = _post_order(session, config, token, "kt10001", payload)
    data["_side"] = "매도"
    data["_order"] = payload
    return data
