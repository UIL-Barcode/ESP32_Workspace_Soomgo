from typing import Any

import asyncio
import discord
from discord.ext import tasks

from adapters.discord.formatters import format_schedule_detail
from bot.confirm import ConfirmManager
from bot.parser import CommandParseError, load_commands, parse_message, validate_handlers
from config.settings import Config
from handlers import build_handlers
from handlers.context import HandlerContext
from handlers.schedule import parse_schedule_tokens
from kiwoom.client import KiwoomClient
from monitor.status_writer import build_status, write_status
from notify.notifier import Notifier
from scheduler.runner import ScheduleRunner
from services import build_services


class DiscordBotRunner:
    def __init__(
        self,
        config: Config,
        kiwoom: KiwoomClient,
        notifier: Notifier,
    ):
        self.config = config
        self.kiwoom = kiwoom
        self.notifier = notifier
        self.commands_data = load_commands(config.commands_file)
        self.services = build_services(kiwoom, notifier, config)
        self.handlers = build_handlers()
        self.confirm = ConfirmManager(timeout_sec=config.confirm_timeout)
        self.ctx = HandlerContext(
            config=config,
            kiwoom=kiwoom,
            notifier=notifier,
            services=self.services,
            commands_data=self.commands_data,
            commands_path=config.commands_file,
        )

        missing = validate_handlers(self.commands_data, self.handlers)
        if missing:
            raise RuntimeError(f"commands.json에 등록되지 않은 handler: {', '.join(missing)}")

        intents = discord.Intents.default()
        intents.message_content = True
        self.bot = discord.Client(intents=intents)
        self._publish_status_loop = tasks.loop(
            seconds=config.status_publish_interval_sec
        )(self._publish_status)
        self._scheduler = ScheduleRunner(self.services.schedule, self.services.run, notifier)
        self._scheduler_loop = tasks.loop(
            seconds=config.scheduler_poll_interval_sec
        )(self._scheduler.tick)
        self._register_events()

    async def _publish_status(self) -> None:
        """대시보드(별도 프로세스)가 읽을 상태 스냅샷을 파일에 기록한다."""
        status = build_status(
            practice_mode=self.config.practice_mode,
            discord_connected=self.bot.is_ready(),
            discord_user=str(self.bot.user) if self.bot.user else "",
            algorithm_count=len(self.services.algo.list_algorithms()),
        )
        await asyncio.to_thread(write_status, self.config.bot_status_file, status)

    def _register_events(self) -> None:
        @self.bot.event
        async def on_ready():
            self.notifier.info(f"디스코드 봇 로그인: {self.bot.user}", to_discord=False)
            if not self._publish_status_loop.is_running():
                self._publish_status_loop.start()
            if not self._scheduler_loop.is_running():
                self._scheduler_loop.start()

        @self.bot.event
        async def on_message(message: discord.Message):
            if message.author.bot:
                return
            if not self.config.is_allowed_discord(message.channel.id, message.author.id):
                return

            content = message.content.strip()

            if self.confirm.is_waiting:
                reply = await self.confirm.handle_reply(message.author.id, content)
                if reply is not None:
                    await message.reply(reply)
                return

            try:
                parsed = parse_message(self.commands_data, content)
            except CommandParseError as e:
                err = str(e)
                if e.example:
                    err += f"\n예시: {e.example}"
                await message.reply(err)
                return

            if parsed is None:
                return

            try:
                if parsed.confirm:
                    try:
                        prompt = await self._build_confirm_prompt(parsed)
                    except Exception as e:
                        self.notifier.error(str(e), to_discord=False)
                        await message.reply(f"오류: {e}")
                        return
                    if prompt is None:
                        prompt = self.confirm.build_prompt(parsed)

                    async def execute() -> str:
                        try:
                            return await self._run_handler(parsed.handler, parsed.kwargs)
                        except Exception as e:
                            self.notifier.error(str(e), to_discord=False)
                            return f"오류: {e}"

                    await message.reply(
                        self.confirm.set_pending(message.author.id, prompt, execute)
                    )
                    return

                result = await self._run_handler(parsed.handler, parsed.kwargs)
                await message.reply(f"```\n{result}\n```")
            except Exception as e:
                self.notifier.error(str(e), to_discord=False)
                await message.reply(f"오류: {e}")

    async def _build_confirm_prompt(self, parsed) -> str | None:
        if parsed.handler == "algo_set_param":
            preview = await asyncio.to_thread(
                self.services.algo.preview_param_change,
                parsed.kwargs["알고리즘"],
                parsed.kwargs["파라미터"],
                str(parsed.kwargs["값"]),
            )
            unit = f" {preview.unit}" if preview.unit else ""
            return (
                f"파라미터 변경: {preview.algorithm_id} · {preview.param_name}: "
                f"{preview.old_value}{unit} → {preview.new_value}{unit}"
            )
        if parsed.handler == "schedule_add":
            fields = await parse_schedule_tokens(self.ctx, parsed.kwargs["tokens"])
            spec = await asyncio.to_thread(self.services.schedule.preview_add, **fields)
            return format_schedule_detail(spec, title="스케줄 등록 확인")
        return None

    async def _run_handler(self, handler_name: str, kwargs: dict[str, Any]) -> str:
        handler = self.handlers.get(handler_name)
        if handler is None:
            raise RuntimeError(f"handler 없음: {handler_name}")
        return await handler(self.ctx, **kwargs)

    def run(self) -> None:
        try:
            self.bot.run(self.config.discord_bot_token)
        except discord.errors.PrivilegedIntentsRequired:
            self.notifier.error(
                "Discord Message Content Intent가 비활성화되어 있습니다.\n"
                "1. https://discord.com/developers/applications/ 접속\n"
                "2. 봇 애플리케이션 선택 → Bot 탭\n"
                "3. Privileged Gateway Intents → "
                "'MESSAGE CONTENT INTENT' 켜기\n"
                "4. 저장 후 python main.py 다시 실행"
            )
            raise SystemExit(1) from None
