import asyncio
from typing import Any

from handlers.context import HandlerContext
from models.symbol import SymbolAmbiguousError, SymbolNotFoundError


async def to_thread(func, *args, **kwargs):
    return await asyncio.to_thread(func, *args, **kwargs)


async def resolve_stock_code(ctx: HandlerContext, raw: str) -> str:
    try:
        return await to_thread(ctx.services.symbol.resolve, raw)
    except SymbolAmbiguousError as e:
        lines = "\n".join(f"  {sym.code} {sym.name}" for sym in e.matches)
        raise RuntimeError(
            f"여러 종목이 일치합니다: {e.query}\n{lines}\n종목코드를 직접 입력하세요."
        ) from e
    except SymbolNotFoundError as e:
        raise RuntimeError(str(e)) from e
