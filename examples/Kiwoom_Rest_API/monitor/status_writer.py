import json
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass
class BotStatus:
    updated_at: str
    practice_mode: bool
    discord_connected: bool
    discord_user: str
    algorithm_count: int


def build_status(
    *,
    practice_mode: bool,
    discord_connected: bool,
    discord_user: str,
    algorithm_count: int,
) -> BotStatus:
    return BotStatus(
        updated_at=datetime.now(timezone.utc).isoformat(),
        practice_mode=practice_mode,
        discord_connected=discord_connected,
        discord_user=discord_user,
        algorithm_count=algorithm_count,
    )


def write_status(path: Path, status: BotStatus) -> None:
    """대시보드 프로세스가 읽는 상태 스냅샷을 원자적으로 기록한다."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with tmp.open("w", encoding="utf-8") as f:
        json.dump(asdict(status), f, ensure_ascii=False, indent=2)
    tmp.replace(path)
