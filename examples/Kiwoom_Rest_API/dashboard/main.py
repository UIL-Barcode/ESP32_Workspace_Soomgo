"""대시보드 진입점 — bot/main.py와 완전히 독립된 프로세스로 실행한다.

봇 프로세스와는 파일(state/bot_status.json, algorithms.json)로만 통신하며
discord.py/kiwoom 세션을 공유하지 않는다 (프로세스 분리 원칙).
"""
import json

from nicegui import ui

from config.settings import Config
from services.config_service import ConfigService  # noqa: F401 (services 패키지를 먼저 초기화해
# algorithm.loader <-> services.algo_service 순환 임포트를 피한다)
from algorithm.loader import AlgoConfig

config = Config.load()
_config_service = ConfigService(config)
_algo_config = AlgoConfig(_config_service, config.algorithms_file)


def _read_bot_status() -> dict:
    if not config.bot_status_file.exists():
        return {}
    try:
        with config.bot_status_file.open(encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError):
        return {}


@ui.page("/")
def index() -> None:
    ui.label("키움 자동매매 봇 - 상태 모니터").classes("text-xl font-bold")
    status_label = ui.label("불러오는 중...")
    algo_label = ui.label("")

    def refresh() -> None:
        status = _read_bot_status()
        if not status:
            status_label.set_text("상태 파일 없음 - 봇이 실행 중인지 확인하세요.")
        else:
            mode = "모의투자" if status.get("practice_mode") else "실전투자"
            connected = "연결됨" if status.get("discord_connected") else "연결 안 됨"
            status_label.set_text(
                f"모드: {mode} | 디스코드: {connected} "
                f"({status.get('discord_user') or '-'}) | "
                f"갱신: {status.get('updated_at', '-')}"
            )

        try:
            _algo_config.reload()
            ids = _algo_config.list_ids()
            algo_label.set_text(f"등록된 알고리즘 ({len(ids)}개): {', '.join(ids) or '-'}")
        except Exception as e:
            algo_label.set_text(f"알고리즘 로드 실패: {e}")

    ui.timer(5.0, refresh)
    refresh()


if __name__ in {"__main__", "__mp_main__"}:
    ui.run(title="Kiwoom Bot Dashboard", reload=False, show=False, port=8081)
