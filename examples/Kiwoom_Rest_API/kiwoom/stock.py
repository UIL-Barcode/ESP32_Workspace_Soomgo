from typing import Any

import requests

from config.settings import Config


def fetch_price(
    session: requests.Session,
    config: Config,
    token: str,
    stock_code: str,
) -> dict[str, Any]:
    url = f"{config.base_url}/api/dostk/stkinfo"
    headers = {
        "Content-Type": "application/json;charset=UTF-8",
        "authorization": f"Bearer {token}",
        "api-id": "ka10001",
    }
    response = session.post(url, headers=headers, json={"stk_cd": stock_code}, timeout=10)
    response.raise_for_status()
    return response.json()
