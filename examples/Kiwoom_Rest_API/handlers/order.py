from typing import Any

from adapters.discord.formatters import format_order_result
from handlers._util import resolve_stock_code, to_thread
from handlers.context import HandlerContext


async def sell_order(ctx: HandlerContext, **kwargs: Any) -> str:
    stock_code = await resolve_stock_code(ctx, kwargs["종목"])
    result = await to_thread(
        ctx.services.order.sell,
        stock_code,
        kwargs["수량"],
        kwargs.get("가격"),
    )
    return format_order_result(result)


async def buy_order(ctx: HandlerContext, **kwargs: Any) -> str:
    stock_code = await resolve_stock_code(ctx, kwargs["종목"])
    result = await to_thread(
        ctx.services.order.buy,
        stock_code,
        kwargs["수량"],
        kwargs.get("가격"),
    )
    return format_order_result(result)
