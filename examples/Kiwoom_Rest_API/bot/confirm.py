import time
from dataclasses import dataclass
from typing import Awaitable, Callable

from bot.parser import ParsedCommand

ConfirmExecutor = Callable[[], Awaitable[str]]


@dataclass
class PendingConfirm:
    user_id: int
    prompt: str
    execute: ConfirmExecutor
    created_at: float


class ConfirmManager:
    YES = "예"
    NO = "아니요"

    def __init__(self, timeout_sec: int = 60):
        self.timeout_sec = timeout_sec
        self._pending: PendingConfirm | None = None

    @property
    def is_waiting(self) -> bool:
        return self._pending is not None

    def set_pending(self, user_id: int, prompt: str, execute: ConfirmExecutor) -> str:
        self._pending = PendingConfirm(user_id=user_id, prompt=prompt, execute=execute, created_at=time.time())
        return f"{prompt}\n`예` 또는 `아니요`로 답해주세요."

    def clear(self) -> None:
        self._pending = None

    def _is_expired(self, pending: PendingConfirm) -> bool:
        return (time.time() - pending.created_at) > self.timeout_sec

    async def handle_reply(self, user_id: int, text: str) -> str | None:
        pending = self._pending
        if pending is None:
            return None

        if pending.user_id != user_id:
            return None

        if self._is_expired(pending):
            self.clear()
            return "확인 시간이 초과되어 취소되었습니다."

        stripped = text.strip()
        if stripped == self.YES:
            self.clear()
            return await pending.execute()
        if stripped == self.NO:
            self.clear()
            return "취소되었습니다."
        return "확인 대기 중입니다. `예` 또는 `아니요`만 입력해주세요."

    def block_message(self) -> str:
        return "확인 대기 중입니다. `예` 또는 `아니요`로 답해주세요."

    def build_prompt(self, cmd: ParsedCommand) -> str:
        parts = [cmd.description or cmd.handler]
        if cmd.kwargs:
            detail = " / ".join(f"{k}: {v}" for k, v in cmd.kwargs.items())
            parts.append(detail)
        return " — ".join(parts)
