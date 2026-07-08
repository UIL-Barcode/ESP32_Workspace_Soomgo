import requests

from config.settings import Config


class Notifier:
    def __init__(self, config: Config):
        self.config = config

    def info(self, msg: str, to_discord: bool = True) -> None:
        print(f"[시스템] {msg}")
        if to_discord:
            self._send_discord(f"ℹ️ {msg}")

    def error(self, msg: str, to_discord: bool = True) -> None:
        print(f"[에러] {msg}")
        if to_discord:
            self._send_discord(f"🚨 [에러] {msg}")

    def _send_discord(self, message: str) -> None:
        if not self.config.discord_alarm or not self.config.webhook_url:
            return
        try:
            response = requests.post(
                self.config.webhook_url,
                json={"content": message},
                timeout=5,
            )
            response.raise_for_status()
        except requests.exceptions.RequestException as e:
            print(f"[디스코드 발송 에러] {e}")
