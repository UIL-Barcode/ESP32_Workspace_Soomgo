import json
import re
from datetime import datetime, timezone
from pathlib import Path

from config.settings import Config
from kiwoom.client import KiwoomClient
from kiwoom.format_util import is_stock_code, normalize_stock_code, normalize_symbol_name
from models.symbol import StockSymbol, SymbolAmbiguousError, SymbolNotFoundError


class SymbolService:
    def __init__(self, kiwoom: KiwoomClient, config: Config):
        self._kiwoom = kiwoom
        self._cache_path = config.stock_symbols_cache
        self._aliases_path = config.symbol_aliases_file
        self._seed_path = config.stock_symbols_seed
        self._cache_max_age_sec = config.symbol_cache_max_age_hours * 3600
        self._symbols: list[StockSymbol] = []
        self._by_code: dict[str, StockSymbol] = {}
        self._by_name: dict[str, list[StockSymbol]] = {}
        self._aliases = self._load_aliases()
        self._loaded = False

    def ensure_loaded(self) -> None:
        if self._loaded:
            return
        if self._try_load_cache():
            self._loaded = True
            return
        self.refresh()

    def refresh(self) -> int:
        self._aliases = self._load_aliases()
        raw_items = self._kiwoom.get_stock_list()
        symbols = self._parse_items(raw_items)
        symbols = self._merge_symbols(symbols)
        self._build_index(symbols)
        self._save_cache(symbols)
        self._loaded = True
        return len(symbols)

    def resolve(self, query: str) -> str:
        self.ensure_loaded()
        text = query.strip()
        if not text:
            raise SymbolNotFoundError(query)

        if is_stock_code(text):
            code = normalize_stock_code(text)
            if code in self._by_code:
                return code
            return code

        normalized = normalize_symbol_name(text)
        alias_target = self._aliases.get(normalized)
        if alias_target:
            text = alias_target
            normalized = normalize_symbol_name(text)

        exact = self._by_name.get(normalized, [])
        if len(exact) == 1:
            return exact[0].code
        if len(exact) > 1:
            raise SymbolAmbiguousError(query, exact)

        prefix_matches = [
            sym
            for key, symbols in self._by_name.items()
            for sym in symbols
            if key.startswith(normalized)
        ]
        prefix_matches = _unique_symbols(prefix_matches)
        if len(prefix_matches) == 1:
            return prefix_matches[0].code
        if len(prefix_matches) > 1:
            raise SymbolAmbiguousError(query, prefix_matches[:15])

        contains_matches = [
            sym
            for key, symbols in self._by_name.items()
            for sym in symbols
            if normalized in key
        ]
        contains_matches = _unique_symbols(contains_matches)
        if len(contains_matches) == 1:
            return contains_matches[0].code
        if len(contains_matches) > 1:
            raise SymbolAmbiguousError(query, contains_matches[:15])

        raise SymbolNotFoundError(query)

    def _parse_items(self, raw_items: list[dict]) -> list[StockSymbol]:
        symbols: list[StockSymbol] = []
        seen: set[str] = set()
        for item in raw_items:
            code_raw = str(item.get("code", "")).strip()
            name = str(item.get("name", "")).strip()
            if not code_raw or not name:
                continue
            if not code_raw.isdigit():
                continue
            code = normalize_stock_code(code_raw)
            if code in seen:
                continue
            seen.add(code)
            symbols.append(
                StockSymbol(
                    code=code,
                    name=name,
                    market_code=str(item.get("marketCode", "")),
                )
            )
        return symbols

    def _build_index(self, symbols: list[StockSymbol]) -> None:
        symbols = self._merge_symbols(symbols)
        self._symbols = symbols
        self._by_code = {sym.code: sym for sym in symbols}
        by_name: dict[str, list[StockSymbol]] = {}
        for sym in symbols:
            key = normalize_symbol_name(sym.name)
            by_name.setdefault(key, []).append(sym)
        self._by_name = by_name

    def _load_seed_symbols(self) -> list[StockSymbol]:
        if not self._seed_path.exists():
            return []
        try:
            with self._seed_path.open(encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            return []
        if not isinstance(data, list):
            return []
        seeds: list[StockSymbol] = []
        for item in data:
            code = str(item.get("code", "")).strip()
            name = str(item.get("name", "")).strip()
            if not code or not name:
                continue
            seeds.append(
                StockSymbol(
                    code=normalize_stock_code(code),
                    name=name,
                    market_code=str(item.get("market_code", "")),
                )
            )
        return seeds

    def _merge_symbols(self, symbols: list[StockSymbol]) -> list[StockSymbol]:
        merged = {sym.code: sym for sym in symbols}
        for seed in self._load_seed_symbols():
            merged.setdefault(seed.code, seed)
        return list(merged.values())

    def _load_aliases(self) -> dict[str, str]:
        if not self._aliases_path.exists():
            return {}
        with self._aliases_path.open(encoding="utf-8") as f:
            data = json.load(f)
        return {normalize_symbol_name(k): v.strip() for k, v in data.items()}

    def _try_load_cache(self) -> bool:
        if not self._cache_path.exists():
            return False
        try:
            with self._cache_path.open(encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            return False

        updated_at = data.get("updated_at")
        if not updated_at:
            return False
        try:
            ts = datetime.fromisoformat(updated_at)
            if ts.tzinfo is None:
                ts = ts.replace(tzinfo=timezone.utc)
            age = (datetime.now(timezone.utc) - ts.astimezone(timezone.utc)).total_seconds()
            if age > self._cache_max_age_sec:
                return False
        except ValueError:
            return False

        raw_symbols = data.get("symbols")
        if not isinstance(raw_symbols, list) or not raw_symbols:
            return False

        symbols = [
            StockSymbol(
                code=str(item["code"]),
                name=str(item["name"]),
                market_code=str(item.get("market_code", "")),
            )
            for item in raw_symbols
            if item.get("code") and item.get("name")
        ]
        if not symbols:
            return False

        self._build_index(symbols)
        return True

    def _save_cache(self, symbols: list[StockSymbol]) -> None:
        self._cache_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "updated_at": datetime.now(timezone.utc).isoformat(),
            "symbols": [
                {
                    "code": sym.code,
                    "name": sym.name,
                    "market_code": sym.market_code,
                }
                for sym in symbols
            ],
        }
        with self._cache_path.open("w", encoding="utf-8") as f:
            json.dump(payload, f, ensure_ascii=False, indent=2)


def _unique_symbols(symbols: list[StockSymbol]) -> list[StockSymbol]:
    seen: set[str] = set()
    unique: list[StockSymbol] = []
    for sym in symbols:
        if sym.code in seen:
            continue
        seen.add(sym.code)
        unique.append(sym)
    return unique
