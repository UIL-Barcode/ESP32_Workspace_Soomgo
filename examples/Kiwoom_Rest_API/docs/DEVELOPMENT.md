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

## Phase 5 — 스케줄 (미착수)

| 모듈 | 파일 | 상태 |
|------|------|------|
| M16 | `schedules.json`, `scheduler/` | `[ ]` |
| M17 | `handlers/schedule.py` | `[ ]` |

---

## 현재 상태

**Phase 4 완료 (M12~M15)** — 다음 작업: **M16** (schedules.json / ScheduleService)

> Phase 4~5 신규 기능은 **Service에 먼저 구현** → handler/GUI는 adapter만 추가

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
