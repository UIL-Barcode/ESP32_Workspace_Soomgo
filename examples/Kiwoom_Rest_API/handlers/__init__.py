from typing import Any

from handlers.context import HandlerContext
from handlers import algo as algo_handler
from handlers import help as help_handler
from handlers import order as order_handler
from handlers import query as query_handler
from handlers import run as run_handler
from handlers import schedule as schedule_handler

HandlerFn = Any


def build_handlers() -> dict[str, HandlerFn]:
    return {
        "get_stock_price": query_handler.get_stock_price,
        "get_holdings": query_handler.get_holdings,
        "refresh_symbols": query_handler.refresh_symbols,
        "algo_list": algo_handler.algo_list,
        "algo_show": algo_handler.algo_show,
        "algo_set_param": algo_handler.algo_set_param,
        "run_live": run_handler.run_live,
        "run_simulation": run_handler.run_simulation,
        "sell_order": order_handler.sell_order,
        "buy_order": order_handler.buy_order,
        "schedule_list": schedule_handler.schedule_list,
        "schedule_add": schedule_handler.schedule_add,
        "schedule_disable": schedule_handler.schedule_disable,
        "schedule_delete": schedule_handler.schedule_delete,
        "show_help": help_handler.show_help,
    }
