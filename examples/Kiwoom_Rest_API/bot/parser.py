import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


class CommandParseError(Exception):
    def __init__(self, message: str, example: str | None = None):
        self.example = example
        super().__init__(message)


@dataclass
class ParsedCommand:
    handler: str
    kwargs: dict[str, Any] = field(default_factory=dict)
    confirm: bool = False
    description: str = ""
    example: str = ""


def load_commands(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


def _is_leaf(node: dict[str, Any]) -> bool:
    return "handler" in node


def _coerce_arg(value: str, arg_type: str) -> Any:
    if arg_type == "int":
        return int(value)
    if arg_type == "float":
        return float(value)
    return value


def _parse_args(arg_defs: list[dict[str, Any]], tokens: list[str]) -> dict[str, Any]:
    kwargs: dict[str, Any] = {}
    idx = 0
    for spec in arg_defs:
        name = spec["name"]
        required = spec.get("required", True)
        if idx < len(tokens):
            kwargs[name] = _coerce_arg(tokens[idx], spec.get("type", "string"))
            idx += 1
        elif required:
            raise CommandParseError(f"필수 인자가 없습니다: {name}")
        elif "default" in spec:
            kwargs[name] = spec["default"]
    if idx < len(tokens):
        raise CommandParseError(f"인자가 너무 많습니다: {' '.join(tokens[idx:])}")
    return kwargs


def _parse_flexible_args(node: dict[str, Any], tokens: list[str]) -> dict[str, Any]:
    """가변 인자: 알고리즘 파라미터 등 (알고리즘 · 파라미터 · 값)."""
    if len(tokens) < 3:
        raise CommandParseError(
            "인자가 부족합니다. 알고리즘, 파라미터, 값 순으로 입력하세요.",
            node.get("example"),
        )
    return {
        "알고리즘": tokens[0],
        "파라미터": " ".join(tokens[1:-1]),
        "값": tokens[-1],
    }


def resolve_command(commands_root: dict[str, Any], tokens: list[str]) -> ParsedCommand:
    if not tokens:
        raise CommandParseError("명령어가 비어 있습니다.", "!도움말")

    node: Any = commands_root
    i = 0

    while i < len(tokens):
        if not isinstance(node, dict):
            break

        if _is_leaf(node):
            break

        token = tokens[i]
        if token in node:
            node = node[token]
            i += 1
        elif "_self" in node:
            node = node["_self"]
        else:
            keys = [k for k in node.keys() if not k.startswith("_")]
            raise CommandParseError(
                f"알 수 없는 명령 경로: '{token}' (가능: {', '.join(keys)})"
            )

    # !도움말, !매수 등 경로만으로 끝나는 _self 명령
    if isinstance(node, dict) and not _is_leaf(node) and "_self" in node:
        node = node["_self"]

    if not isinstance(node, dict) or not _is_leaf(node):
        raise CommandParseError("명령 정의를 찾을 수 없습니다.", "!도움말")

    arg_tokens = tokens[i:]
    try:
        if node.get("arg_mode") == "flexible":
            kwargs = _parse_flexible_args(node, arg_tokens)
        else:
            kwargs = _parse_args(node.get("args", []), arg_tokens)
    except ValueError as e:
        raise CommandParseError(str(e), node.get("example")) from e
    except CommandParseError:
        raise
    except Exception as e:
        raise CommandParseError(str(e), node.get("example")) from e

    return ParsedCommand(
        handler=node["handler"],
        kwargs=kwargs,
        confirm=bool(node.get("confirm", False)),
        description=node.get("description", ""),
        example=node.get("example", ""),
    )


def parse_message(commands_data: dict[str, Any], content: str) -> ParsedCommand | None:
    prefix = commands_data.get("prefix", "!")
    text = content.strip()
    if not text.startswith(prefix):
        return None

    body = text[len(prefix) :].strip()
    if not body:
        raise CommandParseError("명령어가 비어 있습니다.", "!도움말")

    tokens = body.split()
    return resolve_command(commands_data["commands"], tokens)


def validate_handlers(commands_data: dict[str, Any], handlers: dict[str, Any]) -> list[str]:
    missing: list[str] = []

    def walk(node: Any) -> None:
        if not isinstance(node, dict):
            return
        if _is_leaf(node):
            name = node["handler"]
            if name not in handlers:
                missing.append(name)
            return
        for key, child in node.items():
            if key.startswith("_"):
                continue
            walk(child)
        if "_self" in node:
            walk(node["_self"])

    walk(commands_data.get("commands", {}))
    return missing


def iter_command_entries(commands_root: dict[str, Any], prefix: str = "") -> list[tuple[str, dict[str, Any]]]:
    entries: list[tuple[str, dict[str, Any]]] = []

    def walk(node: dict[str, Any], path: str) -> None:
        if _is_leaf(node):
            entries.append((path, node))
            return
        if "_self" in node:
            walk(node["_self"], path)
        for key, child in node.items():
            if key.startswith("_"):
                continue
            child_path = f"{path} {key}".strip()
            walk(child, child_path)

    for key, child in commands_root.items():
        walk(child, key)
    return entries
