from typing import Any

from bot.parser import iter_command_entries
from handlers.context import HandlerContext


async def show_help(ctx: HandlerContext, **kwargs: Any) -> str:
    prefix = ctx.commands_data.get("prefix", "!")
    lines = ["[명령 목록]"]
    for path, node in iter_command_entries(ctx.commands_data["commands"]):
        example = node.get("example") or f"{prefix}{path.replace(' ', ' ')}"
        desc = node.get("description", "")
        lines.append(f"• {example}")
        if desc:
            lines.append(f"  {desc}")
    return "\n".join(lines)
