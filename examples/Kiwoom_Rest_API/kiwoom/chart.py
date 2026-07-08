import time
from typing import Any

import requests

from config.settings import Config
from kiwoom.format_util import parse_amount
from models.candle import Candle

_MAX_RETRIES = 5
_RETRY_BASE_SEC = 1.5
_BETWEEN_PAGE_SEC = 0.5


def fetch_daily_chart(
    session: requests.Session,
    config: Config,
    token: str,
    stock_code: str,
    base_dt: str,
    *,
    adjusted: bool = True,
    max_pages: int = 20,
    until_dt: str | None = None,
) -> list[Candle]:
    """일봉 차트(ka10081). base_dt 이전(포함) 데이터, 연속조회.

    until_dt가 있으면 해당일 이전 봉이 충분히 모이면 연속조회를 중단한다.
    """
    url = f"{config.base_url}/api/dostk/chart"
    payload = {
        "stk_cd": stock_code,
        "base_dt": base_dt,
        "upd_stkpc_tp": "1" if adjusted else "0",
    }
    raw_items: list[dict[str, Any]] = []
    cont_yn = ""
    next_key = ""

    for _ in range(max_pages):
        headers = {
            "Content-Type": "application/json;charset=UTF-8",
            "authorization": f"Bearer {token}",
            "api-id": "ka10081",
        }
        if cont_yn == "Y" and next_key:
            headers["cont-yn"] = cont_yn
            headers["next-key"] = next_key

        data, response = _post_with_retry(session, url, headers=headers, json=payload)
        if data.get("return_code", 0) != 0:
            raise RuntimeError(data.get("return_msg", "일봉 조회 실패"))

        page = data.get("stk_dt_pole_chart_qry") or []
        raw_items.extend(page)

        cont_yn = response.headers.get("cont-yn", data.get("cont-yn", "N"))
        next_key = response.headers.get("next-key", data.get("next-key", ""))
        if cont_yn != "Y" or not next_key:
            break

        if until_dt and page:
            oldest = min(str(item.get("dt", "")) for item in page)
            if oldest and oldest < until_dt:
                break
        time.sleep(_BETWEEN_PAGE_SEC)

    candles = [_parse_daily(item) for item in raw_items]
    candles = [c for c in candles if c.date]
    candles.sort(key=lambda c: c.date)
    # dedupe by date (연속조회 중복 대비)
    deduped: dict[str, Candle] = {c.date: c for c in candles}
    return [deduped[k] for k in sorted(deduped)]


def _parse_daily(item: dict[str, Any]) -> Candle:
    return Candle(
        date=str(item.get("dt", "")).strip(),
        open=abs(parse_amount(item.get("open_pric"))),
        high=abs(parse_amount(item.get("high_pric"))),
        low=abs(parse_amount(item.get("low_pric"))),
        close=abs(parse_amount(item.get("cur_prc"))),
        volume=abs(parse_amount(item.get("trde_qty"))),
    )


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
            time.sleep(_RETRY_BASE_SEC * (attempt + 1))
            continue
        if response.status_code >= 400:
            body = response.text.strip() or "(응답 본문 없음)"
            raise RuntimeError(f"차트 API HTTP {response.status_code}: {body[:200]}")
        return response.json(), response

    assert last_response is not None
    raise RuntimeError("차트 API 호출 제한(429). 잠시 후 다시 시도하세요.")
