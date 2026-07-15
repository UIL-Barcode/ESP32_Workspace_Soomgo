from dataclasses import dataclass


@dataclass
class ScheduleSpec:
    id: str
    enabled: bool
    type: str  # once / recurring
    stock_code: str
    algorithm_id: str
    time: str | None = None  # recurring: "HH:MM"
    days: list[str] | None = None  # recurring: ["mon", ...]
    run_at: str | None = None  # once: "YYYY-MM-DDTHH:MM:SS"
    last_run: str | None = None  # "YYYY-MM-DD HH:MM" (마지막 실행 시각, 중복 실행 방지용)
