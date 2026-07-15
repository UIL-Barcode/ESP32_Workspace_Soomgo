import re
from datetime import datetime
from pathlib import Path

from models.schedule import ScheduleSpec
from scheduler.loader import VALID_DAYS, DEFAULT_WEEKDAYS, ScheduleConfig
from services.algo_service import AlgoService
from services.config_service import ConfigService

_TIME_RE = re.compile(r"^([01]\d|2[0-3]):([0-5]\d)$")


class ScheduleService:
    """스케줄 CRUD (schedules.json)."""

    def __init__(
        self,
        config_service: ConfigService,
        schedules_file: Path,
        algo_service: AlgoService,
    ) -> None:
        self._store = ScheduleConfig(config_service, schedules_file)
        self._store.reload()
        self._algo = algo_service

    def reload(self) -> None:
        self._store.reload()

    def list_schedules(self) -> list[ScheduleSpec]:
        return self._store.list_all()

    def preview_add(
        self,
        *,
        schedule_type: str,
        stock_code: str,
        algorithm_id: str,
        time: str | None = None,
        days: list[str] | None = None,
        run_at: str | None = None,
    ) -> ScheduleSpec:
        """confirm 전 검증·미리보기 (저장하지 않음)."""
        return self._build_spec(
            schedule_id=self._store.next_id(),
            schedule_type=schedule_type,
            stock_code=stock_code,
            algorithm_id=algorithm_id,
            time=time,
            days=days,
            run_at=run_at,
        )

    def add_schedule(
        self,
        *,
        schedule_type: str,
        stock_code: str,
        algorithm_id: str,
        time: str | None = None,
        days: list[str] | None = None,
        run_at: str | None = None,
    ) -> ScheduleSpec:
        spec = self._build_spec(
            schedule_id=self._store.next_id(),
            schedule_type=schedule_type,
            stock_code=stock_code,
            algorithm_id=algorithm_id,
            time=time,
            days=days,
            run_at=run_at,
        )
        self._store.add(spec)
        self._store.save()
        self._store.reload()
        return spec

    def disable_schedule(self, schedule_id: str) -> ScheduleSpec:
        self._store.set_enabled(schedule_id, False)
        self._store.save()
        self._store.reload()
        return self._find(schedule_id)

    def delete_schedule(self, schedule_id: str) -> None:
        self._store.delete(schedule_id)
        self._store.save()
        self._store.reload()

    def mark_fired(self, schedule_id: str, when: datetime) -> None:
        self._store.mark_fired(schedule_id, when)
        self._store.save()

    def _find(self, schedule_id: str) -> ScheduleSpec:
        for spec in self._store.list_all():
            if spec.id == schedule_id:
                return spec
        raise KeyError(f"알 수 없는 스케줄: {schedule_id}")

    def _build_spec(
        self,
        *,
        schedule_id: str,
        schedule_type: str,
        stock_code: str,
        algorithm_id: str,
        time: str | None,
        days: list[str] | None,
        run_at: str | None,
    ) -> ScheduleSpec:
        if schedule_type not in ("once", "recurring"):
            raise ValueError("스케줄 타입은 once 또는 recurring이어야 합니다.")
        self._algo.show_algorithm(algorithm_id)

        if schedule_type == "recurring":
            if not time or not _TIME_RE.match(time):
                raise ValueError("시간 형식이 올바르지 않습니다 (HH:MM).")
            day_list = days or list(DEFAULT_WEEKDAYS)
            invalid = [d for d in day_list if d not in VALID_DAYS]
            if invalid:
                raise ValueError(f"알 수 없는 요일: {', '.join(invalid)}")
            return ScheduleSpec(
                id=schedule_id,
                enabled=True,
                type="recurring",
                stock_code=stock_code,
                algorithm_id=algorithm_id,
                time=time,
                days=day_list,
            )

        if not run_at:
            raise ValueError("run_at(실행 일시)이 필요합니다.")
        return ScheduleSpec(
            id=schedule_id,
            enabled=True,
            type="once",
            stock_code=stock_code,
            algorithm_id=algorithm_id,
            run_at=_normalize_run_at(run_at),
        )


def _normalize_run_at(value: str) -> str:
    text = value.strip()
    for fmt in ("%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M"):
        try:
            dt = datetime.strptime(text, fmt)
            return dt.strftime("%Y-%m-%dT%H:%M:%S")
        except ValueError:
            continue
    raise ValueError("run_at 형식이 올바르지 않습니다 (예: 2026-07-07T09:05).")
