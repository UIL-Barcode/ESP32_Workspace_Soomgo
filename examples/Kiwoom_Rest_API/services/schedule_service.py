"""스케줄 CRUD (Phase 5 구현 예정)."""


class ScheduleService:
    def __init__(self, config_service) -> None:
        self._config_service = config_service

    def list_schedules(self) -> list:
        raise NotImplementedError("Phase 5에서 구현 예정")

    def add_schedule(self, **kwargs) -> dict:
        raise NotImplementedError("Phase 5에서 구현 예정")

    def disable_schedule(self, schedule_id: str) -> dict:
        raise NotImplementedError("Phase 5에서 구현 예정")

    def delete_schedule(self, schedule_id: str) -> dict:
        raise NotImplementedError("Phase 5에서 구현 예정")
