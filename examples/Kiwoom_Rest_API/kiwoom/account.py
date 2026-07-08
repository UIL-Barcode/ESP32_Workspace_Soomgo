from typing import Any

import requests

from config.settings import Config


def _post_acnt(
    session: requests.Session,
    config: Config,
    token: str,
    api_id: str,
    payload: dict[str, Any],
) -> dict[str, Any]:
    url = f"{config.base_url}/api/dostk/acnt"
    headers = {
        "Content-Type": "application/json;charset=UTF-8",
        "authorization": f"Bearer {token}",
        "api-id": api_id,
    }
    response = session.post(url, headers=headers, json=payload, timeout=10)
    response.raise_for_status()
    data = response.json()
    if data.get("return_code", 0) != 0:
        raise RuntimeError(data.get("return_msg", "계좌 조회 실패"))
    return data


def fetch_holdings_live(
    session: requests.Session,
    config: Config,
    token: str,
    exchange: str = "KRX",
) -> dict[str, Any]:
    """체결잔고요청 (kt00005) — 실전투자"""
    data = _post_acnt(session, config, token, "kt00005", {"dmst_stex_tp": exchange})
    data["_api"] = "kt00005"
    return data


def fetch_holdings_practice(
    session: requests.Session,
    config: Config,
    token: str,
    exchange: str = "KRX",
) -> dict[str, Any]:
    """계좌평가현황요청 (kt00004) — 모의투자"""
    data = _post_acnt(
        session,
        config,
        token,
        "kt00004",
        {"qry_tp": "0", "dmst_stex_tp": exchange},
    )
    data["_api"] = "kt00004"
    return data
