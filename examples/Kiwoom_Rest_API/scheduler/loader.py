from datetime import datetime
from pathlib import Path
from typing import Any

from models.schedule import ScheduleSpec
from services.config_service import ConfigService

VALID_DAYS = ("mon", "tue", "wed", "thu", "fri", "sat", "sun")
DEFAULT_WEEKDAYS = ["mon", "tue", "wed", "thu", "fri"]


class ScheduleConfig:
    """schedules.json 로드·저장·reload."""

    def __init__(self, config_service: ConfigService, path: Path):
        self._config_service = config_service
        self._path = path
        self._data: dict[str, Any] = {"schedules": []}

    def reload(self) -> None:
        if self._path.exists():
            self._data = self._config_service.load_json(self._path)
        else:
            self._data = {"schedules": []}

    def save(self) -> None:
        self._config_service.save_json(self._path, self._data)

    @property
    def raw(self) -> dict[str, Any]:
        return self._data

    def list_all(self) -> list[ScheduleSpec]:
        return [_to_spec(item) for item in self._data.get("schedules", [])]

    def get_raw(self, schedule_id: str) -> dict[str, Any]:
        for item in self._data.get("schedules", []):
            if item.get("id") == schedule_id:
                return item
        raise KeyError(f"알 수 없는 스케줄: {schedule_id}")

    def add(self, spec: ScheduleSpec) -> None:
        items = self._data.setdefault("schedules", [])
        if any(i.get("id") == spec.id for i in items):
            raise ValueError(f"이미 존재하는 스케줄 ID: {spec.id}")
        items.append(_to_dict(spec))

    def set_enabled(self, schedule_id: str, enabled: bool) -> None:
        self.get_raw(schedule_id)["enabled"] = enabled

    def mark_fired(self, schedule_id: str, when: datetime) -> None:
        item = self.get_raw(schedule_id)
        item["last_run"] = when.strftime("%Y-%m-%d %H:%M")
        if item.get("type") == "once":
            item["enabled"] = False

    def delete(self, schedule_id: str) -> None:
        items = self._data.get("schedules", [])
        before = len(items)
        self._data["schedules"] = [i for i in items if i.get("id") != schedule_id]
        if len(self._data["schedules"]) == before:
            raise KeyError(f"알 수 없는 스케줄: {schedule_id}")

    def next_id(self) -> str:
        existing = {i.get("id", "") for i in self._data.get("schedules", [])}
        n = 1
        while f"sched_{n:03d}" in existing:
            n += 1
        return f"sched_{n:03d}"


def _to_spec(item: dict[str, Any]) -> ScheduleSpec:
    return ScheduleSpec(
        id=str(item.get("id", "")),
        enabled=bool(item.get("enabled", True)),
        type=str(item.get("type", "")),
        stock_code=str(item.get("stock_code", "")),
        algorithm_id=str(item.get("algorithm_id", "")),
        time=item.get("time"),
        days=item.get("days"),
        run_at=item.get("run_at"),
        last_run=item.get("last_run"),
    )


def _to_dict(spec: ScheduleSpec) -> dict[str, Any]:
    data: dict[str, Any] = {
        "id": spec.id,
        "enabled": spec.enabled,
        "type": spec.type,
        "stock_code": spec.stock_code,
        "algorithm_id": spec.algorithm_id,
    }
    if spec.type == "recurring":
        data["time"] = spec.time
        data["days"] = spec.days or list(DEFAULT_WEEKDAYS)
    else:
        data["run_at"] = spec.run_at
    if spec.last_run:
        data["last_run"] = spec.last_run
    return data
