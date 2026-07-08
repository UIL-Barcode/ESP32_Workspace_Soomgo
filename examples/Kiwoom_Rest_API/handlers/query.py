from typing import Any

from adapters.discord.formatters import format_holdings, format_stock_quote
from handlers._util import resolve_stock_code, to_thread
from handlers.context import HandlerContext


async def get_stock_price(ctx: HandlerContext, **kwargs: Any) -> str:
    stock_input = kwargs["종목"]
    stock_code = await resolve_stock_code(ctx, stock_input)
    quote = await to_thread(ctx.services.query.get_stock_price, stock_code)
    return format_stock_quote(quote)


async def get_holdings(ctx: HandlerContext, **kwargs: Any) -> str:
    snapshot = await to_thread(ctx.services.query.get_holdings)
    return format_holdings(snapshot)


async def refresh_symbols(ctx: HandlerContext, **kwargs: Any) -> str:
    count = await to_thread(ctx.services.symbol.refresh)
    seed_note = ""
    if ctx.config.practice_mode:
        seed_note = (
            "\n모의 API 종목 + seed 목록이 합쳐졌습니다. "
            "seed에 없는 종목은 코드로 직접 입력하세요."
        )
    return f"종목코드 캐시 갱신 완료 ({count:,}건){seed_note}"
