from typing import Any

from algorithm.loader import AlgoConfig
from config.settings import Config
from models.algo import AlgorithmSpec, ParamChangeResult
from services.config_service import ConfigService


class AlgoService:
    def __init__(self, config_service: ConfigService, config: Config) -> None:
        self._config_service = config_service
        self._store = AlgoConfig(config_service, config.algorithms_file)
        self._store.reload()

    def reload(self) -> None:
        self._store.reload()

    def list_algorithms(self) -> list[AlgorithmSpec]:
        return [self._store.get_algorithm(algo_id) for algo_id in self._store.list_ids()]

    def show_algorithm(self, algorithm_id: str) -> AlgorithmSpec:
        return self._store.get_algorithm(algorithm_id)

    def set_param(
        self,
        algorithm_id: str,
        param_name: str,
        raw_value: str,
    ) -> ParamChangeResult:
        spec = self._store.get_algorithm(algorithm_id)
        if param_name not in spec.params:
            raise ValueError(f"알 수 없는 파라미터: {param_name}")

        param = spec.params[param_name]
        if not param.writable:
            raise ValueError(f"변경할 수 없는 파라미터입니다: {param_name}")

        new_value = _coerce_value(raw_value, param.type)
        old_value = self._store.set_param_value(algorithm_id, param_name, new_value)
        self._store.save()
        self._store.reload()

        return ParamChangeResult(
            algorithm_id=algorithm_id,
            param_name=param_name,
            old_value=old_value,
            new_value=new_value,
            unit=param.unit,
        )

    def preview_param_change(
        self,
        algorithm_id: str,
        param_name: str,
        raw_value: str,
    ) -> ParamChangeResult:
        """confirm 전 검증·미리보기 (저장하지 않음)."""
        spec = self._store.get_algorithm(algorithm_id)
        if param_name not in spec.params:
            raise ValueError(f"알 수 없는 파라미터: {param_name}")

        param = spec.params[param_name]
        if not param.writable:
            raise ValueError(f"변경할 수 없는 파라미터입니다: {param_name}")

        new_value = _coerce_value(raw_value, param.type)
        return ParamChangeResult(
            algorithm_id=algorithm_id,
            param_name=param_name,
            old_value=param.value,
            new_value=new_value,
            unit=param.unit,
        )


def _coerce_value(raw: str, param_type: str) -> Any:
    text = raw.strip()
    if param_type == "int":
        try:
            return int(text)
        except ValueError as e:
            raise ValueError("정수(int)여야 합니다.") from e
    if param_type == "float":
        try:
            return float(text)
        except ValueError as e:
            raise ValueError("실수(float)여야 합니다.") from e
    return text
