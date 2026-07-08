from typing import Any

from adapters.discord.formatters import (
    format_algorithm_detail,
    format_algorithm_list,
    format_param_change,
)
from handlers._util import to_thread
from handlers.context import HandlerContext


async def algo_list(ctx: HandlerContext, **kwargs: Any) -> str:
    algorithms = await to_thread(ctx.services.algo.list_algorithms)
    return format_algorithm_list(algorithms)


async def algo_show(ctx: HandlerContext, **kwargs: Any) -> str:
    algo_id = kwargs["알고리즘"]
    try:
        algo = await to_thread(ctx.services.algo.show_algorithm, algo_id)
    except KeyError as e:
        raise RuntimeError(str(e)) from e
    return format_algorithm_detail(algo)


async def algo_set_param(ctx: HandlerContext, **kwargs: Any) -> str:
    try:
        result = await to_thread(
            ctx.services.algo.set_param,
            kwargs["알고리즘"],
            kwargs["파라미터"],
            str(kwargs["값"]),
        )
    except (KeyError, ValueError) as e:
        raise RuntimeError(str(e)) from e
    ctx.notifier.info(
        f"알고리즘 파라미터 변경: {result.algorithm_id} · "
        f"{result.param_name} {result.old_value} → {result.new_value}"
    )
    return format_param_change(result)
