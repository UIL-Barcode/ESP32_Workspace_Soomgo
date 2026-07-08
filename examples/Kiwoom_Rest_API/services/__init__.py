from dataclasses import dataclass

from config.settings import Config
from kiwoom.client import KiwoomClient
from notify.notifier import Notifier
from services.algo_service import AlgoService
from services.config_service import ConfigService
from services.order_service import OrderService
from services.query_service import QueryService
from services.run_service import RunService
from services.schedule_service import ScheduleService
from services.symbol_service import SymbolService


@dataclass
class AppServices:
    """UI(Discord/GUI) 공통 비즈니스 레이어."""

    query: QueryService
    order: OrderService
    algo: AlgoService
    schedule: ScheduleService
    run: RunService
    config: ConfigService
    symbol: SymbolService


def build_services(
    kiwoom: KiwoomClient,
    notifier: Notifier,
    config: Config,
) -> AppServices:
    config_service = ConfigService(config)
    algo_service = AlgoService(config_service, config)
    return AppServices(
        query=QueryService(kiwoom, config.practice_mode),
        order=OrderService(kiwoom, notifier, config),
        algo=algo_service,
        schedule=ScheduleService(config_service),
        run=RunService(kiwoom, algo_service, config.output_dir),
        config=config_service,
        symbol=SymbolService(kiwoom, config),
    )
