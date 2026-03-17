# 🚀 ESP32 Internal Framework (Soomgo Mentoring)

본 레포지토리는 ESP32(ESP-IDF) 기반의 하드웨어 제어 및 펌웨어 개발을 위한 **사내 표준 프레임워크 템플릿 및 멘토링 워크스페이스**입니다.

단순한 실습 코드를 넘어, 향후 사내 개발팀이 즉시 도입하여 사용할 수 있는 **[모듈화, 재사용성, 문서화]**를 목표로 설계되었습니다.

---

## 📂 Repository Architecture (폴더 구조)

팀 단위 개발 및 인수인계의 효율성을 극대화하기 위해 아래와 같은 표준 구조를 따릅니다.

* **`docs/`** : 부품 데이터시트(PDF), 회로도, 레슨 교안 및 API 규격서
* **`driverlib/`** : (Core Asset) 자체 제작 하드웨어 제어 드라이버
  * `include/` : 헤더 파일(`.h`) - 함수의 선언부 및 사용법 (팀원 참조용)
  * `src/` : 소스 파일(`.c`) - 내부 구동 로직 (은닉화)
* **`templates/`** : 신규 프로젝트 시작 시 복사해서 사용하는 표준 보일러플레이트(Boilerplate)
* **`examples/`** : 드라이버 사용법을 익히기 위한 모범 샘플 코드 (교육용)
* **`projects/`** : 실제 멘토링 과정에서 진행되는 실습 및 최종 프로젝트 과제
* **`software/`** : 💻 (Premium) 자체 개발 PC 연동 유틸리티 및 제어 프로그램
  * *⚠️ **보안 및 라이선스 안내:** 본 폴더 내의 프로그램들은 본 프레임워크(driverlib)에 최적화된 엔터프라이즈급 제어 툴입니다. 지적 재산권 보호 및 수강생 전용 혜택을 위해 **개인 라이선스 인증 로직(DLL)**이 적용되어 있으며, 제공된 라이선스 파일이 동일 경로에 위치해야만 정상 구동됩니다.*
---

## ⚙️ Development Environment (개발 환경)

본 프로젝트는 개인별 SDK 설치 경로가 달라도 충돌 없이 협업할 수 있도록 구성되어 있습니다.

* **Target Board:** ESP32 Series
* **Framework:** Official ESP-IDF (C/C++)
* **IDE:** Visual Studio Code (ESP-IDF Extension 활용)
* **Version Control:** Git / GitHub

### ⚠️ 빌드 및 협업 시 주의사항
1. **SDK 경로 독립성:** ESP-IDF는 환경 변수(`IDF_PATH`)를 통해 동작하므로, 멘토와 멘티의 로컬 PC 설치 경로가 달라도 소스코드(Git)는 충돌하지 않습니다. 경로 수정 없이 그대로 빌드하시면 됩니다.
2. **Build 폴더:** 컴파일 시 생성되는 `build/` 폴더는 개인 PC의 환경에 종속되므로 Git에 업로드되지 않도록 `.gitignore`에 처리되어 있습니다.
3. **새 프로젝트 시작:** `projects/` 폴더 내에 새로운 실습을 만드실 때는 직접 폴더를 구성하지 마시고, 반드시 `templates/` 폴더의 구조를 복사하여 사용해 주시기 바랍니다.

---

## 📞 Communication
* **Mentor:** 김윤성 (Soomgo)
* **Session:** 매주 지정된 멘토링 시간 및 GitHub Issue 탭 활용