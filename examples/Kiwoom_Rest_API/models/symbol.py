from dataclasses import dataclass


@dataclass(frozen=True)
class StockSymbol:
    code: str
    name: str
    market_code: str = ""


class SymbolNotFoundError(Exception):
    def __init__(self, query: str):
        self.query = query
        super().__init__(f"종목을 찾을 수 없습니다: {query}")


class SymbolAmbiguousError(Exception):
    def __init__(self, query: str, matches: list[StockSymbol]):
        self.query = query
        self.matches = matches
        super().__init__(f"여러 종목이 일치합니다: {query}")
