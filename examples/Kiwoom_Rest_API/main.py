from config.settings import Config
from kiwoom.client import KiwoomClient
from notify.notifier import Notifier
from bot.discord_bot import DiscordBotRunner


def main() -> None:
    config = Config.load()
    notifier = Notifier(config)

    mode = "모의투자" if config.practice_mode else "실전투자"
    notifier.info(f"{mode} 모드로 실행합니다.")

    if not config.discord_bot_token:
        notifier.error("DISCORD_BOT_TOKEN이 설정되지 않았습니다.")
        return

    kiwoom = KiwoomClient(config, notifier)
    if not kiwoom.authenticate():
        notifier.error("키움 API 인증 실패")
        return

    DiscordBotRunner(config, kiwoom, notifier).run()


if __name__ == "__main__":
    main()
