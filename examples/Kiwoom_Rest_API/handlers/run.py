from typing import Any

from adapters.discord.formatters import format_live_signal, format_simulation_result
from handlers._util import resolve_stock_code, to_thread
from handlers.context import HandlerContext


async def run_live(ctx: HandlerContext, **kwargs: Any) -> str:
    stock_code = await resolve_stock_code(ctx, kwargs["종목"])
    algorithm_id = kwargs["알고리즘"]
    try:
        signal = await to_thread(
            ctx.services.run.run_live,
            stock_code,
            algorithm_id,
        )
    except (KeyError, ValueError, RuntimeError) as e:
        raise RuntimeError(str(e)) from e
    return format_live_signal(stock_code, algorithm_id, signal)


async def run_simulation(ctx: HandlerContext, **kwargs: Any) -> str:
    stock_code = await resolve_stock_code(ctx, kwargs["종목"])
    algorithm_id = kwargs["알고리즘"]
    try:
        result = await to_thread(
            ctx.services.run.run_simulation,
            stock_code,
            algorithm_id,
            kwargs["시작일"],
            kwargs["종료일"],
        )
    except (KeyError, ValueError, RuntimeError) as e:
        raise RuntimeError(str(e)) from e
    ctx.notifier.info(
        f"시뮬 완료: {result.stock_code} {result.algorithm_id} "
        f"→ {result.excel_path}"
    )
    return format_simulation_result(result)
