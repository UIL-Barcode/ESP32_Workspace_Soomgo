# 키움 REST API 자동매매 봇 — 설계계획서

## 1. 프로젝트 이해 요약

| 항목 | 내용 |
|------|------|
| **목적** | 키움증권 REST API로 주식 조회·주문을 하고, **디스코드 봇**으로 원격 제어·알림 |
| **운영 방식** | 사설 디스코드 채널 + 개인 봇, **양방향 통신** (명령 입력 ↔ 결과/알림 출력) |
| **핵심 설계 원칙** | 명령·알고리즘·스케줄 설정은 **코드가 아닌 JSON 파일**로 관리 |
| **투자 모드** | 기본 모의투자(`mockapi.kiwoom.com`), 설정으로 실전 전환 |
| **주문** | 디스코드 명령으로 매수/매도 (`예`/`아니요` 확인 후 실행) |
| **알고리즘** | 전략 정의·파라미터는 `algorithms.json`, 실행 시점·종목은 `schedules.json` |
| **시뮬 결과** | **엑셀 파일로 로컬 저장** (디스코드로 결과 본문 미전송) |
| **파라미터 튜닝** | 디스코드 `!알고리즘` 명령 → 저장 후 **즉시 reload** |

### JSON 파일 역할 (3분리)

| 파일 | 역할 |
|------|------|
| **`commands.json`** | 디스코드 명령 체계 (무엇을 호출할지) |
| **`algorithms.json`** | 알고리즘 ID, 전략 handler, 파라미터 정의 |
| **`schedules.json`** | 종목 + algorithm_id + 실행 시간 + `once`/`recurring` |

---

## 2. 시스템 아키텍처

```
[Discord / (향후 GUI)]
        │
        ▼
  UI Adapters          handlers/*  │  adapters/discord/*
  (명령 파싱, confirm, 문자열 포맷)
        │
        ▼
  services/  ◀── UI 공통 비즈니스 레이어
  QueryService, OrderService, AlgoService,
  ScheduleService, RunService, ConfigService
        │
        ▼
  kiwoom/  │  algorithm/  │  scheduler/
  + JSON (algorithms.json, schedules.json)
```

**데이터 흐름**

1. 사용자가 디스코드에 명령 입력 (`!조회 종목 005930` 등)
2. **범용 메시지 리스너**가 접두사(`!`) 이후 토큰 분리
3. **`commands.json` 트리**를 따라 명령 경로·인자 해석
4. `confirm: true`이면 **`예`/`아니요` 확인 후** handler 실행
5. **handler** → **Service** 호출 → 구조화 결과(`models/`) 반환
6. **Discord adapter**가 문자열 포맷 후 응답 (GUI는 동일 Service → 위젯)
7. Phase 4~5: Algo/Schedule/Run Service → 시뮬·스케줄 실행

### Service 레이어 (Discord / GUI 공통)

| Service | 담당 | 상태 |
|---------|------|------|
| **QueryService** | 현재가, 보유 | 구현됨 |
| **OrderService** | 매수, 매도 | 구현됨 |
| **AlgoService** | 알고리즘 조회·튜닝 | Phase 4 |
| **ScheduleService** | 스케줄 CRUD | Phase 5 |
| **RunService** | 현재 실행, 과거 시뮬 | Phase 4 |
| **ConfigService** | JSON 로드·저장 | 기본 구현 |

---

## 3. 디렉터리 구조

```
Kiwoom_Rest_API/
├── main.py                      # 진입점
├── config/
│   └── settings.py              # .env 기반 Config
├── commands.json                # ★ 디스코드 명령 정의
├── algorithms.json              # ★ 알고리즘·파라미터 정의
├── schedules.json               # ★ 실행 스케줄 (once / recurring)
├── 키움 REST API 문서.xlsx       # API 레퍼런스 (TR별 시트)
├── 키움 REST API 문서.pdf        # API 레퍼런스 (PDF)
├── bot/
│   ├── discord_bot.py
│   ├── parser.py                # JSON 트리 파싱 + 인자 검증
│   └── confirm.py               # 예/아니요 확인 (순차 처리)
├── models/                      # Service 반환 dataclass (UI 공통)
│   ├── query.py                 # StockQuote, HoldingsSnapshot
│   └── order.py                 # OrderResult
├── services/                    # ★ UI 공통 비즈니스 레이어
│   ├── query_service.py
│   ├── order_service.py
│   ├── algo_service.py          # Phase 4
│   ├── schedule_service.py      # Phase 5
│   ├── run_service.py           # Phase 4
│   └── config_service.py
├── adapters/
│   └── discord/
│       └── formatters.py        # Discord용 문자열 포맷
├── handlers/                    # Discord thin adapter (Service 호출)
│   ├── __init__.py              # HANDLERS 레지스트리
│   ├── query.py
│   ├── order.py
│   ├── algo.py                  # Phase 4
│   ├── schedule.py              # Phase 5
│   ├── run.py                   # Phase 4
│   └── help.py
├── algorithm/
│   ├── loader.py                # AlgoConfig (algorithms.json)
│   ├── registry.py              # STRATEGIES 레지스트리
│   └── strategies/              # 전략 로직
├── scheduler/
│   ├── loader.py                # ScheduleConfig (schedules.json)
│   └── runner.py                # once/recurring 실행 루프
├── kiwoom/
│   ├── client.py
│   ├── auth.py
│   ├── stock.py
│   ├── account.py               # 보유/잔고
│   ├── order.py
│   └── chart.py                 # 일봉/분봉 (Phase 4)
├── notify/
│   └── notifier.py
├── output/                      # 시뮬 엑셀 출력 (gitignore)
├── .env
├── .env.example
├── .gitignore
├── requirements.txt
└── README.md
```

- **자주 수정**: `commands.json`, `algorithms.json`, `schedules.json`
- **가끔 수정**: `handlers/`, `kiwoom/`, `algorithm/strategies/`
- **거의 안 수정**: `bot/parser.py`, `main.py`

---

## 4. `commands.json` 스키마 설계

### 4.1 설계 원칙

| 원칙 | 설명 |
|------|------|
| **트리 = 명령 경로** | 1차·2차·3차는 라우팅용 |
| **리프 = 실행 메타** | `handler`, `args`, `description`, `example`, `confirm` |
| **인자 ≠ 깊은 트리** | `가격`, `수량` 등은 리프의 `args` 정의 |
| **1단 명령** | `매도`, `매수` 등은 `"_self"` 키 사용 |
| **가변 인자** | `알고리즘 파라미터` 등은 `arg_mode: "flexible"` 로 전용 파싱 |

### 4.2 스키마 예시

```json
{
  "prefix": "!",
  "commands": {
    "조회": {
      "종목": {
        "handler": "get_stock_price",
        "description": "종목 현재가 조회",
        "args": [
          { "name": "종목코드", "type": "string", "required": true }
        ],
        "example": "!조회 종목 005930"
      },
      "보유": {
        "handler": "get_holdings",
        "description": "보유 종목 조회",
        "args": [],
        "example": "!조회 보유"
      }
    },
    "매도": {
      "_self": {
        "handler": "sell_order",
        "description": "매도 주문",
        "confirm": true,
        "args": [
          { "name": "종목코드", "type": "string", "required": true },
          { "name": "수량", "type": "int", "required": true },
          { "name": "가격", "type": "int", "required": false, "default": null }
        ],
        "example": "!매도 005930 10"
      }
    },
    "매수": {
      "_self": {
        "handler": "buy_order",
        "description": "매수 주문",
        "confirm": true,
        "args": [
          { "name": "종목코드", "type": "string", "required": true },
          { "name": "수량", "type": "int", "required": true },
          { "name": "가격", "type": "int", "required": false, "default": null }
        ],
        "example": "!매수 005930 10 72000"
      }
    },
    "실행": {
      "현재": {
        "handler": "run_live",
        "description": "현재 시점 데이터로 알고리즘 신호 확인",
        "args": [
          { "name": "종목코드", "type": "string", "required": true },
          { "name": "알고리즘", "type": "string", "required": true }
        ],
        "example": "!실행 현재 005930 이동평균"
      },
      "과거": {
        "handler": "run_simulation",
        "description": "과거 구간 시뮬 → 엑셀 저장",
        "args": [
          { "name": "종목코드", "type": "string", "required": true },
          { "name": "알고리즘", "type": "string", "required": true },
          { "name": "시작일", "type": "string", "required": true },
          { "name": "종료일", "type": "string", "required": true }
        ],
        "example": "!실행 과거 005930 이동평균 20240101 20241231"
      }
    },
    "스케줄": {
      "목록": {
        "handler": "schedule_list",
        "description": "등록된 스케줄 목록",
        "args": [],
        "example": "!스케줄 목록"
      },
      "등록": {
        "handler": "schedule_add",
        "description": "스케줄 등록 (once / recurring)",
        "confirm": true,
        "arg_mode": "flexible",
        "example": "!스케줄 등록 recurring 005930 이동평균 09:05"
      },
      "중지": {
        "handler": "schedule_disable",
        "description": "스케줄 비활성화",
        "confirm": true,
        "args": [
          { "name": "스케줄ID", "type": "string", "required": true }
        ],
        "example": "!스케줄 중지 sched_001"
      },
      "삭제": {
        "handler": "schedule_delete",
        "description": "스케줄 삭제",
        "confirm": true,
        "args": [
          { "name": "스케줄ID", "type": "string", "required": true }
        ],
        "example": "!스케줄 삭제 sched_001"
      }
    },
    "알고리즘": {
      "목록": {
        "handler": "algo_list",
        "description": "등록된 알고리즘 목록",
        "args": [],
        "example": "!알고리즘 목록"
      },
      "조회": {
        "handler": "algo_show",
        "description": "알고리즘 파라미터 조회",
        "args": [
          { "name": "알고리즘", "type": "string", "required": true }
        ],
        "example": "!알고리즘 조회 이동평균"
      },
      "파라미터": {
        "handler": "algo_set_param",
        "description": "파라미터 값 변경",
        "confirm": true,
        "arg_mode": "flexible",
        "example": "!알고리즘 파라미터 이동평균 단기 10"
      }
    },
    "도움말": {
      "_self": {
        "handler": "show_help",
        "description": "명령 목록 출력",
        "args": [],
        "example": "!도움말"
      }
    }
  }
}
```

### 4.3 파싱 규칙

```
입력:  !조회 종목 005930
토큰:  ["조회", "종목", "005930"]
경로:  commands["조회"]["종목"]  → 리프
인자:  { "종목코드": "005930" }

입력:  !실행 과거 005930 이동평균 20240101 20241231
경로:  commands["실행"]["과거"]
인자:  { "종목코드": "005930", "알고리즘": "이동평균", "시작일": "20240101", "종료일": "20241231" }
```

- 트리를 따라가다 **`_self`이거나 리프(`handler` 존재)** 이면 종료
- `arg_mode: "flexible"` 명령은 handler 또는 parser 전용 로직으로 인자 해석
- 실패 시 `example`을 포함한 에러 메시지 반환

### 4.4 디스코드 명령 요약

| 분류 | 명령 예시 | 저장 | 결과 출력 |
|------|-----------|------|-----------|
| 조회 | `!조회 종목 005930` | — | 디스코드 |
| 주문 | `!매도 005930 10` | — | 디스코드 |
| 즉시 실행 | `!실행 현재 005930 이동평균` | — | 디스코드 (요약) |
| 시뮬 | `!실행 과거 005930 이동평균 20240101 20241231` | — | **엑셀** (`output/`) |
| 스케줄 | `!스케줄 등록 recurring ...` | `schedules.json` | 디스코드 (등록 확인) |
| 알고리즘 | `!알고리즘 파라미터 이동평균 단기 10` | `algorithms.json` | 디스코드 (변경 확인) |

---

## 5. `algorithms.json` 스키마 설계

알고리즘 **정의와 파라미터만** 관리한다. 어떤 종목·언제 실행할지는 `schedules.json`이 담당한다. **`active` 필드는 사용하지 않는다.**

### 5.1 설계 원칙

| 원칙 | 설명 |
|------|------|
| **역할** | algorithm_id, 설명, strategy handler, params 정의 |
| **검증** | **기술적 검증만** (타입 파싱, JSON 형식) — 비즈니스 규칙(단기<장기 등)은 사람이 시뮬 결과로 판단 |
| **튜닝** | 디스코드 `!알고리즘` → 저장 → **즉시 reload** |
| **writable** | `false`인 파라미터는 디스코드에서 변경 불가 |

### 5.2 스키마 예시

```json
{
  "algorithms": {
    "이동평균": {
      "description": "단기/장기 이동평균 크로스",
      "strategy": "moving_average",
      "params": {
        "단기": { "value": 5, "type": "int", "writable": true },
        "장기": { "value": 20, "type": "int", "writable": true },
        "손절": { "value": 3.0, "type": "float", "unit": "%", "writable": true }
      }
    },
    "RSI": {
      "description": "RSI 과매수/과매도",
      "strategy": "rsi",
      "params": {
        "기간": { "value": 14, "type": "int", "writable": true },
        "과매도": { "value": 30, "type": "int", "writable": true },
        "과매수": { "value": 70, "type": "int", "writable": true }
      }
    }
  }
}
```

### 5.3 파라미터 튜닝 명령

| 명령 | 동작 |
|------|------|
| `!알고리즘 목록` | 등록된 알고리즘 ID 목록 |
| `!알고리즘 조회 이동평균` | 해당 알고리즘 파라미터 출력 |
| `!알고리즘 파라미터 이동평균 단기 10` | 파라미터 변경 (`예`/`아니요` 확인) |

**변경 처리 흐름**

```
!알고리즘 파라미터 이동평균 단기 10
  → confirm: 이동평균 · 단기: 5 → 10. `예` 또는 `아니요`?
  → 예: 타입 검증 → algorithms.json 저장 (atomic write) → AlgoConfig reload
  → 디스코드: "변경 완료" (웹훅 알림 선택)
```

**검증 (기술적만)**

| 검증 | 실패 시 |
|------|---------|
| `writable: false` | 디스코드에서 변경 불가 안내 |
| 타입 불일치 | `정수(int)여야 합니다` 등 |
| 없는 알고리즘/파라미터 | `알 수 없는 알고리즘/파라미터` |

> 파라미터 조합의 유효성(예: 단기 > 장기)은 **시뮬 결과를 보고 사람이 판단** 후 `schedules.json`에 등록 여부를 결정한다.

---

## 6. `schedules.json` 스키마 설계

**종목 + algorithm_id + 실행 시점**을 관리한다. `type`은 `once` 또는 `recurring`만 사용한다.

### 6.1 스키마 예시

```json
{
  "schedules": [
    {
      "id": "sched_001",
      "enabled": true,
      "type": "recurring",
      "stock_code": "005930",
      "algorithm_id": "이동평균",
      "time": "09:05",
      "days": ["mon", "tue", "wed", "thu", "fri"]
    },
    {
      "id": "sched_002",
      "enabled": true,
      "type": "once",
      "stock_code": "005930",
      "algorithm_id": "RSI",
      "run_at": "2026-07-07T09:05:00"
    }
  ]
}
```

| 필드 | once | recurring |
|------|------|-----------|
| `type` | `"once"` | `"recurring"` |
| 실행 시각 | `run_at` (ISO datetime) | `time` (HH:MM) + `days` |
| 용도 | 1회 실행 | 요일·시간 반복 (장중 자동) |

### 6.2 스케줄 명령

| 명령 | 동작 |
|------|------|
| `!스케줄 목록` | 등록된 스케줄 조회 |
| `!스케줄 등록 recurring 005930 이동평균 09:05` | 반복 스케줄 추가 |
| `!스케줄 등록 once 005930 RSI 2026-07-07T09:05` | 1회 스케줄 추가 |
| `!스케줄 중지 sched_001` | `enabled: false` |
| `!스케줄 삭제 sched_001` | 항목 제거 |

- 등록·중지·삭제는 `confirm: true` + 저장 후 **ScheduleConfig 즉시 reload**
- Scheduler가 `recurring`/`once` 항목을 시간에 맞춰 실행

### 6.3 권장 워크플로

```
1. !알고리즘 파라미터 ... 로 튜닝
2. !실행 과거 ... 로 시뮬 → output/ 엑셀 확인
3. 사람이 결과 검토 후 적용 여부 결정
4. !스케줄 등록 ... 으로 자동 실행 등록
```

---

## 7. Handler / Strategy 레지스트리 (분리)

디스코드 명령 handler와 전략 로직을 **별도 레지스트리**로 관리한다.

```python
# handlers/__init__.py — HANDLERS (commands.json)
def build_handlers(kiwoom, notifier, algo_config, schedule_config) -> dict:
    return {
        "get_stock_price":  query.get_stock_price,
        "get_holdings":     query.get_holdings,
        "sell_order":       order.sell_order,
        "buy_order":        order.buy_order,
        "show_help":        help.show_help,
        "run_live":         run.run_live,
        "run_simulation":   run.run_simulation,
        "schedule_list":    schedule.schedule_list,
        "schedule_add":     schedule.schedule_add,
        "schedule_disable": schedule.schedule_disable,
        "schedule_delete":  schedule.schedule_delete,
        "algo_list":        algo.algo_list,
        "algo_show":        algo.algo_show,
        "algo_set_param":   algo.algo_set_param,
    }

# algorithm/registry.py — STRATEGIES (algorithms.json)
STRATEGIES = {
    "moving_average": strategies.moving_average,
    "rsi":            strategies.rsi,
}
```

- **새 디스코드 명령**: `commands.json` + HANDLERS 1줄
- **새 알고리즘 종류**: `algorithms.json` + `strategies/` 1개 + STRATEGIES 1줄

---

## 8. 디스코드 봇 설계

### 8.1 통신 방식

| 방향 | 수단 | 용도 |
|------|------|------|
| **입력** | `on_message` | 명령 수신 |
| **출력 (응답)** | `message.reply` | 조회·주문·짧은 확인 |
| **출력 (알림)** | 웹훅 (선택) | 시스템 로그, 에러 |
| **시뮬 결과** | `output/*.xlsx` | 엑셀만 (디스코드 미전송, 완료 메시지만) |

### 8.2 확인(confirm) 플로우

```
[사용자] !매도 005930 10
[봇]     매도 확인 — 종목: 005930 / 수량: 10주
         `예` 또는 `아니요`로 답해주세요.
[사용자] 예
[봇]     주문 접수 완료
```

**규칙**

| 규칙 | 내용 |
|------|------|
| **순차 처리** | confirm 대기 중 **새 명령 무시** (다른 명령 처리 안 함) |
| **인식 문자** | `예`, `아니요` **만** 인정 (`y`, `네`, `ㅛ` 등 불가) |
| **오타** | `예` 또는 `아니요`만 입력 가능하다고 안내, **계속 대기** |
| **타임아웃** | (선택) N초 후 자동 `아니요` 처리 |

```
[사용자] !매도 005930 10
[봇]     confirm 대기
[사용자] !조회 종목 005930
[봇]     확인 대기 중입니다. `예` 또는 `아니요`로 답해주세요.
```

- `confirm.py`: `user_id → pending_confirm` 1건 유지

### 8.3 보안·운영

- **채널 ID** / **사용자 ID** 각 1개 (단수, 확장 없음)
- `.env`에 값이 있으면 해당 ID만 허용, 비어 있으면 전체 허용 (또는 구현 시 `.env.example`에 명시)
- 실전 모드(`PRACTICE_MODE=0`) 시 추가 경고 문구
- `.env`는 git 제외, `.env.example` 제공

---

## 9. 키움 API 레이어

### 9.1 API 레퍼런스 문서

프로젝트 루트에 키움 공식 문서를 포함한다.

| 파일 | 용도 |
|------|------|
| `키움 REST API 문서.xlsx` | TR별 시트 (api-id, URL, 요청/응답 필드) |
| `키움 REST API 문서.pdf` | 동일 내용 PDF |

구현 시 `api-id`·payload는 위 문서를 기준으로 `kiwoom/` 모듈에 캡슐화한다.

### 9.2 공통

- `KiwoomClient`: `requests.Session`, 토큰, 공통 헤더 (`authorization`, `api-id`)
- `PRACTICE_MODE`에 따라 `base_url` / 키 분기
- 401 응답 시 토큰 재발급 후 재시도

### 9.3 Phase별 TR 매핑 (문서 기준)

| Phase | 용도 | API ID | URL |
|-------|------|--------|-----|
| 1 | OAuth2 토큰 | `au10001` | `/oauth2/token` |
| 1 | 종목 기본정보/현재가 | `ka10001` | `/api/dostk/stkinfo` |
| 2 | 체결잔고/보유 | `kt00005` 등 | `/api/dostk/acnt` |
| 3 | 주식 매수 | `kt10000` | `/api/dostk/ordr` |
| 3 | 주식 매도 | `kt10001` | `/api/dostk/ordr` |
| 4 | 주식 일봉 차트 | `ka10081` | `/api/dostk/chart` |
| 4 | 주식 분봉 차트 | `ka10080` | `/api/dostk/chart` |

> 상세 요청/응답 필드는 `키움 REST API 문서.xlsx` 해당 시트 참조.

---

## 10. 설정 (`.env`)

```env
# 투자 모드 (1=모의, 0=실전)
PRACTICE_MODE=1
PRACTICE_KIWOOM_APP_KEY=
PRACTICE_KIWOOM_APP_SECRET=
KIWOOM_APP_KEY=
KIWOOM_APP_SECRET=

# 디스코드
DISCORD_BOT_TOKEN=
DISCORD_ALARM=1
WEBHOOK_URL=

# 접근 제어 (단수, 선택)
DISCORD_CHANNEL_ID=
DISCORD_USER_ID=

# JSON 설정 파일
COMMANDS_FILE=commands.json
ALGORITHMS_FILE=algorithms.json
SCHEDULES_FILE=schedules.json

# confirm 타임아웃 (초, 선택)
CONFIRM_TIMEOUT=60
```

### JSON reload 정책

| 변경 경로 | reload |
|-----------|--------|
| 디스코드 `!알고리즘`, `!스케줄` | 저장 후 **즉시 메모리 reload** |
| 파일 직접 편집 | 봇 **재시작** (또는 추후 `!설정 reload` 추가) |

---

## 11. 개발 단계 (로드맵)

### Phase 1 — 골격 + JSON 명령 체계

- [ ] 프로젝트 구조 생성
- [ ] `Config`, `Notifier`, `KiwoomClient`, `CommandParser`
- [ ] `commands.json` + 범용 `on_message` + `confirm` (`예`/`아니요`)
- [ ] `get_stock_price` + `!조회 종목` + `show_help`
- [ ] `.env.example`, `.gitignore`

**완료 기준**: JSON만 수정해 명령 변경 가능, `!조회 종목` 동작

### Phase 2 — 조회 확장

- [ ] `get_holdings` (`kt00005` 등, xlsx 문서 참조)
- [ ] 채널/사용자 ID 화이트리스트 (단수)

### Phase 3 — 디스코드 주문 (모의투자)

- [ ] `kt10000`/`kt10001` 매수·매도 연동
- [ ] `!매수`/`!매도` + confirm

**완료 기준**: 모의투자 API로 디스코드 주문 가능

### Phase 4 — 알고리즘·시뮬·엑셀

- [ ] `algorithms.json` + `AlgoConfig`
- [ ] `!알고리즘` 튜닝 + 즉시 reload
- [ ] `!실행 현재` / `!실행 과거` + STRATEGIES
- [ ] 차트 API (`ka10080`, `ka10081`) + 시뮬 엔진
- [ ] 결과 **엑셀** 출력 (`output/`)

**완료 기준**: 파라미터 튜닝 → 과거 시뮬 → 엑셀 확인 (디스코드에 결과 본문 없음)

### Phase 5 — 스케줄 자동 실행

- [ ] `schedules.json` + `ScheduleConfig`
- [ ] `!스케줄` CRUD + 즉시 reload
- [ ] `scheduler/runner.py` (`once` / `recurring`)

**완료 기준**: 스케줄 등록 후 지정 시간에 알고리즘 자동 실행

---

## 12. 기술 스택

| 구분 | 선택 |
|------|------|
| 언어 | Python 3.11 |
| 디스코드 | discord.py 2.x |
| HTTP | requests |
| 설정 | python-dotenv |
| 데이터 | pandas, openpyxl |
| 기타 | websockets, PySide6 — 가상환경에 미리 설치, 당장 미사용 |

---

## 13. 리스크·결정 사항

| 항목 | 방향 |
|------|------|
| 주문·스케줄·파라미터 변경 | `confirm: true`, `예`/`아니요`만 |
| confirm 대기 중 | 새 명령 처리 안 함 |
| handler 오타 | 기동 시 JSON ↔ HANDLERS 검증 |
| 파라미터 검증 | 기술적(타입)만 — 비즈니스 판단은 사람+시뮬 |
| JSON 동시 쓰기 | atomic write |
| API 블로킹 | `asyncio.to_thread` |
| 시뮬 결과 | 엑셀 로컬만, 디스코드 미전송 |
| 실전 전환 | `.env` + 봇 경고 |

---

## 14. 개발 환경 설정

### 사전 준비

1. Python 3.11 설치
2. 키움증권 개발자 센터에서 App Key / Secret Key 발급
3. 디스코드 봇 생성 및 토큰 발급 (선택: 웹훅 URL)
4. API 상세는 프로젝트 루트 `키움 REST API 문서.xlsx` / `.pdf` 참조

### venv

```bash
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

### Anaconda

```bash
conda env create -f environment.yml
conda activate kiwoom_env
```

### 실행

```bash
python main.py
```

---

## 15. 체크리스트

- [x] 키움 REST API + 디스코드 양방향 봇
- [x] `commands.json` / `algorithms.json` / `schedules.json` 3분리
- [x] `schedules.json`: `once` / `recurring`만 사용
- [x] Handler(STRATEGIES) / Strategy 레지스트리 분리
- [x] confirm: 순차 처리, `예`/`아니요`만
- [x] 시뮬 결과: 엑셀 로컬 저장 (디스코드 미전송)
- [x] 튜닝·스케줄 변경: 저장 후 즉시 reload
- [x] 파라미터 비즈니스 검증은 사람이 시뮬로 판단
- [x] API 문서: 프로젝트 루트 xlsx/pdf
- [x] 채널·사용자 ID 단수 화이트리스트
