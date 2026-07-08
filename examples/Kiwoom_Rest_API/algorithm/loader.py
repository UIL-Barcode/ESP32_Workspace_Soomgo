from pathlib import Path
from typing import Any

from models.algo import AlgorithmSpec, ParamSpec
from services.config_service import ConfigService


class AlgoConfig:
    """algorithms.json 로드·저장·reload."""

    def __init__(self, config_service: ConfigService, path: Path):
        self._config_service = config_service
        self._path = path
        self._data: dict[str, Any] = {}

    def reload(self) -> None:
        self._data = self._config_service.load_algorithms()

    def save(self) -> None:
        self._config_service.save_json(self._path, self._data)

    @property
    def raw(self) -> dict[str, Any]:
        return self._data

    def list_ids(self) -> list[str]:
        algorithms = self._data.get("algorithms", {})
        return sorted(algorithms.keys())

    def get_algorithm_raw(self, algorithm_id: str) -> dict[str, Any]:
        algorithms = self._data.get("algorithms", {})
        if algorithm_id not in algorithms:
            raise KeyError(f"알 수 없는 알고리즘: {algorithm_id}")
        return algorithms[algorithm_id]

    def get_algorithm(self, algorithm_id: str) -> AlgorithmSpec:
        raw = self.get_algorithm_raw(algorithm_id)
        params: dict[str, ParamSpec] = {}
        for name, spec in (raw.get("params") or {}).items():
            params[name] = ParamSpec(
                name=name,
                value=spec.get("value"),
                type=str(spec.get("type", "string")),
                writable=bool(spec.get("writable", True)),
                unit=str(spec.get("unit", "")),
            )
        return AlgorithmSpec(
            algorithm_id=algorithm_id,
            description=str(raw.get("description", "")),
            strategy=str(raw.get("strategy", "")),
            params=params,
        )

    def set_param_value(self, algorithm_id: str, param_name: str, value: Any) -> Any:
        raw = self.get_algorithm_raw(algorithm_id)
        params = raw.get("params") or {}
        if param_name not in params:
            raise KeyError(f"알 수 없는 파라미터: {param_name}")
        old_value = params[param_name].get("value")
        params[param_name]["value"] = value
        return old_value
