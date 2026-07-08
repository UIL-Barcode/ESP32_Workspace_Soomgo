import time
from typing import Any

import requests

from config.settings import Config

_MARKET_TYPES = ("0", "1")  # 0: 코스피, 1: 코스닥
_MAX_RETRIES = 5
_RETRY_BASE_SEC = 1.5
_BETWEEN_MARKET_SEC = 2.0
_BETWEEN_PAGE_SEC = 0.5


def fetch_stock_list(
    session: requests.Session,
    config: Config,
    token: str,
) -> list[dict[str, Any]]:
    """종목코드 리스트(ka10101) — 코스피·코스닥 전체."""
    all_items: list[dict[str, Any]] = []
    for idx, market_type in enumerate(_MARKET_TYPES):
        if idx > 0:
            time.sleep(_BETWEEN_MARKET_SEC)
        all_items.extend(_fetch_market(session, config, token, market_type))
    return all_items


def _fetch_market(
    session: requests.Session,
    config: Config,
    token: str,
    market_type: str,
) -> list[dict[str, Any]]:
    url = f"{config.base_url}/api/dostk/stkinfo"
    items: list[dict[str, Any]] = []
    cont_yn = ""
    next_key = ""

    while True:
        headers = {
            "Content-Type": "application/json;charset=UTF-8",
            "authorization": f"Bearer {token}",
            "api-id": "ka10101",
        }
        if cont_yn == "Y" and next_key:
            headers["cont-yn"] = cont_yn
            headers["next-key"] = next_key

        data, response = _post_with_retry(
            session,
            url,
            headers=headers,
            json={"mrkt_tp": market_type},
        )
        if data.get("return_code", 0) != 0:
            raise RuntimeError(data.get("return_msg", "종목코드 리스트 조회 실패"))

        items.extend(data.get("list") or [])

        cont_yn = response.headers.get("cont-yn", data.get("cont-yn", "N"))
        next_key = response.headers.get("next-key", data.get("next-key", ""))
        if cont_yn != "Y" or not next_key:
            break
        time.sleep(_BETWEEN_PAGE_SEC)

    return items


def _post_with_retry(
    session: requests.Session,
    url: str,
    *,
    headers: dict[str, str],
    json: dict[str, Any],
) -> tuple[dict[str, Any], requests.Response]:
    last_response: requests.Response | None = None
    for attempt in range(_MAX_RETRIES):
        response = session.post(url, headers=headers, json=json, timeout=30)
        last_response = response

        if response.status_code == 429:
            wait = _RETRY_BASE_SEC * (attempt + 1)
            time.sleep(wait)
            continue

        if response.status_code >= 400:
            body = response.text.strip() or "(응답 본문 없음)"
            raise RuntimeError(
                f"종목코드 API HTTP {response.status_code}: {body[:200]}"
            )

        return response.json(), response

    assert last_response is not None
    raise RuntimeError(
        "종목코드 API 호출 제한(429 Too Many Requests). "
        "잠시 후 !조회 종목갱신 을 다시 시도하세요."
    )
