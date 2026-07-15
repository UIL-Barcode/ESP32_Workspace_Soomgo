# 개발 모듈 계획 (재개용)

> 토큰 초과·세션 끊김 시: **미완료 `[ ]` 모듈부터** 이어서 진행.
> 각 모듈은 **독립 커밋 단위**로 완료 가능하게 나눔.

## 의존성 순서

```
M0 문서/환경 → M1 config → M2 notify → M3 kiwoom
    → M4 commands.json → M5 parser → M6 confirm
    → M7 handlers → M8 discord_bot → M9 main
```

## Phase 1 — 골격 + JSON 명령 (현재)

| 모듈 | 파일 | 상태 | 완료 기준 |
|------|------|------|-----------|
| **M0** | `docs/DEVELOPMENT.md`, `.gitignore`, `.env.example` | `[x]` | 재개 문서·gitignore 존재 |
| **M1** | `config/settings.py` | `[x]` | `.env` 로드, 경로·화이트리스트 |
| **M2** | `notify/notifier.py` | `[x]` | 터미널 + 웹훅 |
| **M3** | `kiwoom/client.py`, `auth.py`, `stock.py` | `[x]` | 토큰 발급 + ka10001 현재가 |
| **M4** | `commands.json` | `[x]` | 조회·도움말·매수/매도 스키마(주문 handler는 스텁) |
| **M5** | `bot/parser.py` | `[x]` | 트리 파싱, args 매핑, handler 검증 |
| **M6** | `bot/confirm.py` | `[x]` | 예/아니요, 순차 처리, 타임아웃 |
| **M7** | `handlers/` | `[x]` | query, help, registry, context |
| **M8** | `bot/discord_bot.py` | `[x]` | on_message 라우팅 |
| **M9** | `main.py` | `[x]` | 진입점만, 예제 코드 제거 |

## Phase 2 — 조회 확장

| 모듈 | 파일 | 상태 |
|------|------|------|
| M10 | `kiwoom/account.py`, `handlers/query.py` | `[x]` | kt00005 보유 조회 |

## Phase 3 — 주문 (미착수)

| 모듈 | 파일 | 상태 |
|------|------|------|
| M11 | `kiwoom/order.py`, `handlers/order.py` | `[x]` | kt10000/kt10001 |

## Phase 3.5 — Service 레이어 분리

| 모듈 | 파일 | 상태 |
|------|------|------|
| M11.5 | `models/`, `services/`, `adapters/discord/` | `[x]` | Query/Order Service 분리, GUI 대비 골격 |

## Phase 4 — 알고리즘·시뮬

| 모듈 | 파일 | 상태 |
|------|------|------|
| M12 | `algorithms.json`, `algorithm/loader.py`, `AlgoService` | `[x]` |
| M13 | `handlers/algo.py`, `commands.json` 알고리즘 명령 | `[x]` |
| M14 | `algorithm/registry.py`, `strategies/` | `[x]` |
| M15 | `kiwoom/chart.py`, `RunService`, 엑셀 출력, `!실행` | `[x]` |
| M15.1 | `Strategy.warmup_bars()` (전략별 자체 선언, `RunService` 하드코딩 제거) | `[x]` |
| M15.2 | `SimulationResult`/`TradeEvent`에 손익 지표 추가 (누적수익률·승률·MDD·라운드트립) — `sim_export.py`, `formatters.py` 반영 | `[x]` |

## Phase 5 — 스케줄

| 모듈 | 파일 | 상태 | 비고 |
|------|------|------|------|
| M16 | `schedules.json`, `scheduler/loader.py`(ScheduleConfig), `scheduler/runner.py`(ScheduleRunner), `services/schedule_service.py` | `[x]` | once/recurring, `RunService.run_live` 자동 실행(신호만, **실주문 없음**) |
| M17 | `handlers/schedule.py`, `commands.json` 스케줄 명령(목록/등록/중지/삭제) | `[x]` | 등록은 `arg_mode: raw` — recurring/once 형태가 달라 기존 flexible 파서 재사용 안 함 |

## Phase 6 — 모니터링 대시보드 (신규, 별도 프로세스)

> README 설계 시점엔 없던 축. 자원 사용량 우선(호스트에 렌더링 부담 없는 구조) +
> PyInstaller 패키징 시 `multiprocessing` freeze 함정 회피를 위해
> **봇 프로세스와 대시보드 프로세스를 완전히 분리**하고 파일(`state/bot_status.json`,
> `algorithms.json`)로만 통신하는 구조로 결정.

| 모듈 | 파일 | 상태 | 비고 |
|------|------|------|------|
| M18 | `monitor/status_writer.py`, `config/settings.py`(`bot_status_file`, `status_publish_interval_sec`) | `[x]` | 원자적 쓰기 (`config_service.py` 패턴 재사용) |
| M19 | `bot/discord_bot.py` — `discord.ext.tasks.loop`로 주기 상태 퍼블리시 | `[x]` | 봇 프로세스엔 서버/스레드 추가 없음 |
| M20 | `dashboard/main.py` (NiceGUI, `python -m dashboard.main`) | `[x]` | 상태 파일 + `algorithms.json` 읽기만 하는 최소 골격. `kiwoom_rest_env`로 렌더링 실측 검증 완료 |
| M21 | 대시보드 요구사항 도출 (관심종목 예측가·알고리즘 결과 시각화, 화면 요소 편집 기능) | `[ ]` | **보류 중** — 요구사항 먼저 정리 후 위젯 착수 (프레임워크는 NiceGUI로 결정됨) |
| M22 | PyInstaller 패키징 (봇 exe / 대시보드 exe 분리, Option A) | `[ ]` | 미착수 |

---

## 알려진 이슈 / 기술 부채

| 항목 | 내용 | 상태 |
|------|------|------|
| 순환 임포트 | `algorithm/loader.py` ↔ `services` 패키지(`services/__init__.py` → `algo_service` → `algorithm.loader`) — 기존 진입점(`main.py`)은 항상 `services`를 먼저 import해서 우연히 안 터지지만, `algorithm.loader`를 먼저 import하는 새 진입점(예: `dashboard/main.py`)에서는 `ImportError` 발생. `dashboard/main.py`는 import 순서 조정으로 우회만 해둔 상태 | `[ ]` 근본 수정 필요 (백그라운드 작업으로 분리됨) |
| 지표 계산 벡터화 | `moving_average.py`/`rsi.py`가 매 봉마다 슬라이스 재계산(O(n·period)), `pandas`가 의존성엔 있지만 실사용 안 함 — 볼린저밴드/MACD 등 무거운 지표 추가 전 벡터화 권장 | `[ ]` 필요 시 착수 |
| 분봉 데이터 계층 | `kiwoom/chart.py`는 일봉(`ka10081`)만 구현, 분봉(`ka10080`) 미구현 — 스캘핑류 알고리즘 추가 시 선행 필요 | `[ ]` 필요 시 착수 |

## 현재 상태

**Phase 4(성과지표 보강 포함) + Phase 5(스케줄러) + 모니터링 대시보드 골격(M18~M20) 완료**
다음 작업 후보: **M21**(대시보드 요구사항 도출, 보류 중) / **M22**(PyInstaller)

> 스케줄 실행은 `!실행 현재`와 동일하게 신호 계산 + 알림만 수행하고 실제 매수/매도 주문은 내지 않는다
> (자동 주문은 confirm 우회 + 자금 이동이라는 별도 안전성 판단이 필요해 의도적으로 범위 밖으로 둠).

> Phase 4~5 신규 기능은 **Service에 먼저 구현** → handler/GUI는 adapter만 추가
> 개발 환경: `python`/`pip` 명령은 반드시 `C:\Users\YoonSungKim\anaconda3\envs\kiwoom_rest_env\python.exe`로 실행 (전역 Python 아님, `.vscode/settings.json`에 반영됨)

## Phase 5 스모크 테스트 (스케줄)

```text
!스케줄 등록 recurring 005930 이동평균 09:05 mon,tue,wed,thu,fri  → 예 → sched_001 등록
!스케줄 등록 once 005930 RSI 2026-07-07T09:05                   → 예 → sched_002 등록
!스케줄 목록      → 등록된 스케줄 + 마지막 실행 시각 확인
!스케줄 중지 sched_001  → 예 → enabled=false
!스케줄 삭제 sched_001  → 예 → 목록에서 제거
  → recurring: 지정 요일·시각에 SCHEDULER_POLL_INTERVAL_SEC(기본 30초) 주기로 자동 실행 감지
  → once: run_at 경과 시 1회 실행 후 자동 비활성화(enabled=false)
```

## Phase 6 스모크 테스트 (대시보드, 별도 프로세스)

```text
python -m dashboard.main   → http://localhost:8081 접속
  → 봇 미실행 시: "상태 파일 없음" + algorithms.json 목록만 표시
  → 봇 실행 중(on_ready 이후, 최대 STATUS_PUBLISH_INTERVAL_SEC 초 내): 모드/디스코드 연결 상태 갱신
```

## Phase 4 스모크 테스트 (실행)

```text
!실행 현재 삼성전자 이동평균
!실행 과거 005930 이동평균 20240101 20241231
  → Discord에는 요약만, 상세는 output/*.xlsx
```

## Phase 4 스모크 테스트 (알고리즘)

```text
!알고리즘 목록
!알고리즘 조회 이동평균
!알고리즘 파라미터 이동평균 단기 10  → 예 → 변경 확인
!알고리즘 조회 이동평균              → 단기: 10 확인
```

## Phase 3 스모크 테스트

```text
!매수 005930 1        → 예 → 시장가 매수
!매도 005930 1        → 예 → 시장가 매도 (보유 시)
!매수 005930 1 70000  → 예 → 지정가 매수
```

## 재개 체크리스트

1. `docs/DEVELOPMENT.md`에서 `[ ]`인 **가장 낮은 M번호** 확인
2. 해당 모듈 파일만 열어 작업
3. 완료 시 `[x]`로 변경
4. `python main.py`로 Phase 1 스모크 테스트

## Phase 2 스모크 테스트

```text
!조회 보유
```

## Phase 1 스모크 테스트

```text
!도움말
!조회 종목 005930
!매도 005930 10  → 예/아니요 확인 후 주문
```
