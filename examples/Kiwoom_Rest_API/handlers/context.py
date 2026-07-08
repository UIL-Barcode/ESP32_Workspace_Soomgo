from dataclasses import dataclass
from pathlib import Path
from typing import Any

from config.settings import Config
from kiwoom.client import KiwoomClient
from notify.notifier import Notifier
from services import AppServices


@dataclass
class HandlerContext:
    config: Config
    kiwoom: KiwoomClient
    notifier: Notifier
    services: AppServices
    commands_data: dict[str, Any]
    commands_path: Path
