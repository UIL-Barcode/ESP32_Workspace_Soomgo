from typing import Any

from adapters.discord.formatters import format_schedule_detail, format_schedule_list
from handlers._util import resolve_stock_code, to_thread
from handlers.context import HandlerContext


async def schedule_list(ctx: HandlerContext, **kwargs: Any) -> str:
    schedules = await to_thread(ctx.services.schedule.list_schedules)
    return format_schedule_list(schedules)


async def schedule_add(ctx: HandlerContext, **kwargs: Any) -> str:
    fields = await parse_schedule_tokens(ctx, kwargs["tokens"])
    try:
        spec = await to_thread(ctx.services.schedule.add_schedule, **fields)
    except (KeyError, ValueError) as e:
        raise RuntimeError(str(e)) from e
    ctx.notifier.info(f"스케줄 등록: {spec.id} · {spec.stock_code} · {spec.algorithm_id}")
    return format_schedule_detail(spec, title="스케줄 등록 완료")


async def schedule_disable(ctx: HandlerContext, **kwargs: Any) -> str:
    schedule_id = kwargs["스케줄ID"]
    try:
        spec = await to_thread(ctx.services.schedule.disable_schedule, schedule_id)
    except KeyError as e:
        raise RuntimeError(str(e)) from e
    return format_schedule_detail(spec, title="스케줄 비활성화 완료")


async def schedule_delete(ctx: HandlerContext, **kwargs: Any) -> str:
    schedule_id = kwargs["스케줄ID"]
    try:
        await to_thread(ctx.services.schedule.delete_schedule, schedule_id)
    except KeyError as e:
        raise RuntimeError(str(e)) from e
    return f"스케줄 삭제 완료: {schedule_id}"


async def parse_schedule_tokens(ctx: HandlerContext, tokens: list[str]) -> dict[str, Any]:
    """!스케줄 등록 raw 토큰 -> ScheduleService.add_schedule/preview_add kwargs."""
    if len(tokens) < 4:
        raise RuntimeError(
            "인자가 부족합니다.\n"
            "recurring: !스케줄 등록 recurring 종목 알고리즘 HH:MM [요일1,요일2,...]\n"
            "once     : !스케줄 등록 once 종목 알고리즘 YYYY-MM-DDTHH:MM"
        )

    schedule_type = tokens[0].strip().lower()
    stock_code = await resolve_stock_code(ctx, tokens[1])
    algorithm_id = tokens[2]

    if schedule_type == "recurring":
        return {
            "schedule_type": "recurring",
            "stock_code": stock_code,
            "algorithm_id": algorithm_id,
            "time": tokens[3],
            "days": tokens[4].split(",") if len(tokens) > 4 else None,
        }
    if schedule_type == "once":
        return {
            "schedule_type": "once",
            "stock_code": stock_code,
            "algorithm_id": algorithm_id,
            "run_at": tokens[3],
        }
    raise RuntimeError("스케줄 타입은 once 또는 recurring이어야 합니다.")
