from typing import Any

import re


def parse_amount(value: Any) -> int:
    if value is None:
        return 0
    text = str(value).strip()
    if not text:
        return 0
    sign = -1 if text.startswith("-") else 1
    digits = text.lstrip("+-") or "0"
    return sign * int(digits)


def parse_rate(value: Any) -> str:
    if value is None:
        return "0"
    text = str(value).strip()
    return text or "0"


def format_won(value: Any) -> str:
    return f"{parse_amount(value):,}"


def normalize_stock_code(stk_cd: str) -> str:
    code = stk_cd.strip()
    if len(code) > 6 and code[0].isalpha():
        return code[1:]
    if code.isdigit():
        return code.zfill(6)
    return code


def is_stock_code(value: str) -> bool:
    text = value.strip()
    if len(text) > 6 and text[0].isalpha():
        text = text[1:]
    return text.isdigit() and 1 <= len(text) <= 6


def normalize_symbol_name(name: str) -> str:
    text = re.sub(r"\s+", "", name.strip())
    return text.lower()
