import asyncio
from datetime import datetime

from adapters.discord.formatters import format_live_signal
from notify.notifier import Notifier
from services.run_service import RunService
from services.schedule_service import ScheduleService

_DAY_NAMES = ("mon", "tue", "wed", "thu", "fri", "sat", "sun")


class ScheduleRunner:
    """schedules.json의 once/recurring 항목을 폴링해 지정 시간에 알고리즘을 실행한다.

    !실행 현재와 동일하게 신호만 계산해 알림으로 전달한다 — 실제 매매 주문은 내지 않는다
    (자동 주문은 confirm 없이 자금이 움직이는 별도 결정이 필요해 범위 밖으로 둠).
    """

    def __init__(
        self,
        schedule_service: ScheduleService,
        run_service: RunService,
        notifier: Notifier,
    ) -> None:
        self._schedule = schedule_service
        self._run = run_service
        self._notifier = notifier

    async def tick(self, now: datetime | None = None) -> None:
        now = now or datetime.now()
        self._schedule.reload()
        for spec in self._schedule.list_schedules():
            if spec.enabled and self._is_due(spec, now):
                await self._fire(spec.id, spec.stock_code, spec.algorithm_id, now)

    def _is_due(self, spec, now: datetime) -> bool:
        stamp = now.strftime("%Y-%m-%d %H:%M")
        if spec.last_run == stamp:
            return False
        if spec.type == "once":
            if not spec.run_at:
                return False
            try:
                target = datetime.fromisoformat(spec.run_at)
            except ValueError:
                return False
            return now >= target
        if spec.type == "recurring":
            if _DAY_NAMES[now.weekday()] not in (spec.days or []):
                return False
            return now.strftime("%H:%M") == spec.time
        return False

    async def _fire(
        self, schedule_id: str, stock_code: str, algorithm_id: str, now: datetime
    ) -> None:
        try:
            signal = await asyncio.to_thread(
                self._run.run_live, stock_code, algorithm_id
            )
            message = format_live_signal(stock_code, algorithm_id, signal)
            self._notifier.info(f"[스케줄 {schedule_id}] 자동 실행\n{message}")
        except Exception as e:
            self._notifier.error(f"[스케줄 {schedule_id}] 자동 실행 실패: {e}")
        finally:
            await asyncio.to_thread(self._schedule.mark_fired, schedule_id, now)
