from dataclasses import dataclass, field
from typing import Any


@dataclass
class ParamSpec:
    name: str
    value: Any
    type: str
    writable: bool = True
    unit: str = ""


@dataclass
class AlgorithmSpec:
    algorithm_id: str
    description: str
    strategy: str
    params: dict[str, ParamSpec] = field(default_factory=dict)


@dataclass
class ParamChangeResult:
    algorithm_id: str
    param_name: str
    old_value: Any
    new_value: Any
    unit: str = ""
