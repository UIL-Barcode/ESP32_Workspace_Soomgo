import os
from dataclasses import dataclass
from pathlib import Path

from dotenv import load_dotenv

_ROOT = Path(__file__).resolve().parent.parent


@dataclass
class Config:
    practice_mode: bool
    base_url: str
    app_key: str
    app_secret: str
    discord_alarm: bool
    webhook_url: str
    discord_bot_token: str
    discord_channel_id: str
    discord_user_id: str
    commands_file: Path
    algorithms_file: Path
    schedules_file: Path
    stock_symbols_cache: Path
    symbol_aliases_file: Path
    stock_symbols_seed: Path
    output_dir: Path
    symbol_cache_max_age_hours: int
    confirm_timeout: int

    @classmethod
    def load(cls) -> "Config":
        load_dotenv(_ROOT / ".env")

        is_practice = os.environ.get("PRACTICE_MODE", "1") == "1"
        if is_practice:
            base_url = "https://mockapi.kiwoom.com"
            app_key = os.environ.get("PRACTICE_KIWOOM_APP_KEY", "")
            app_secret = os.environ.get("PRACTICE_KIWOOM_APP_SECRET", "")
        else:
            base_url = "https://api.kiwoom.com"
            app_key = os.environ.get("KIWOOM_APP_KEY", "")
            app_secret = os.environ.get("KIWOOM_APP_SECRET", "")

        commands_name = (
            os.environ.get("COMMANDS_FILE")
            or os.environ.get("COMMAND_JSON_PATH")
            or "commands.json"
        )

        return cls(
            practice_mode=is_practice,
            base_url=base_url,
            app_key=app_key,
            app_secret=app_secret,
            discord_alarm=os.environ.get("DISCORD_ALARM", "0") == "1",
            webhook_url=os.environ.get("WEBHOOK_URL", ""),
            discord_bot_token=os.environ.get("DISCORD_BOT_TOKEN", ""),
            discord_channel_id=os.environ.get("DISCORD_CHANNEL_ID", ""),
            discord_user_id=os.environ.get("DISCORD_USER_ID", ""),
            commands_file=_ROOT / commands_name,
            algorithms_file=_ROOT / os.environ.get("ALGORITHMS_FILE", "algorithms.json"),
            schedules_file=_ROOT / os.environ.get("SCHEDULES_FILE", "schedules.json"),
            stock_symbols_cache=_ROOT / os.environ.get(
                "STOCK_SYMBOLS_CACHE", "data/stock_symbols.json"
            ),
            symbol_aliases_file=_ROOT / os.environ.get(
                "SYMBOL_ALIASES_FILE", "data/symbol_aliases.json"
            ),
            stock_symbols_seed=_ROOT / os.environ.get(
                "STOCK_SYMBOLS_SEED", "data/stock_symbols_seed.json"
            ),
            output_dir=_ROOT / os.environ.get("OUTPUT_DIR", "output"),
            symbol_cache_max_age_hours=int(
                os.environ.get("SYMBOL_CACHE_MAX_AGE_HOURS", "24")
            ),
            confirm_timeout=int(os.environ.get("CONFIRM_TIMEOUT", "60")),
        )

    def is_allowed_discord(self, channel_id: int, user_id: int) -> bool:
        if self.discord_channel_id and str(channel_id) != self.discord_channel_id:
            return False
        if self.discord_user_id and str(user_id) != self.discord_user_id:
            return False
        return True
