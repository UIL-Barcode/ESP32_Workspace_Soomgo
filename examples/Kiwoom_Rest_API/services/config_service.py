import json
from pathlib import Path
from typing import Any

from config.settings import Config


class ConfigService:
    """JSON 설정 파일 로드·저장 (Phase 4~5에서 확장)."""

    def __init__(self, config: Config):
        self._config = config

    def load_json(self, path: Path) -> dict[str, Any]:
        with path.open(encoding="utf-8") as f:
            return json.load(f)

    def save_json(self, path: Path, data: dict[str, Any]) -> None:
        tmp = path.with_suffix(path.suffix + ".tmp")
        with tmp.open("w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        tmp.replace(path)

    def load_commands(self) -> dict[str, Any]:
        return self.load_json(self._config.commands_file)

    def load_algorithms(self) -> dict[str, Any]:
        return self.load_json(self._config.algorithms_file)

    def load_schedules(self) -> dict[str, Any]:
        return self.load_json(self._config.schedules_file)
