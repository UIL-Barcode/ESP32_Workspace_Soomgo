/*
 * ====================================================================================
 * [필수 하드웨어 설정 가이드 - Classic Bluetooth (SPP)] 
 * * 주의: 본 코드는 안드로이드 앱과의 시리얼 통신을 위해 클래식 블루투스를 사용합니다.
 * 컴파일 전 반드시 sdkconfig에서 아래 설정을 켜주어야 하며, 
 * 설정 누락 시 app_main() 진입 단계에서 ESP_ERR_INVALID_ARG 에러와 함께 무한 재부팅됩니다.
 * (참고: ESP32-S2, S3, C3 계열은 클래식 블루투스를 하드웨어적으로 지원하지 않습니다.)
 * ====================================================================================
 * * [메뉴얼 셋업 순서]
 * 1. vscode 하단 ESP-IDF 아이콘들 중 Clean 아이콘 좌측에 있는 SDK Configuration Editor(톱니바퀴 아이콘) 실행
 * * 2. 블루투스 기능 켜기:
 * -> [Component config] 
 * -> [Bluetooth] 
 * -> 'Bluetooth' 항목 스페이스바를 눌러 체크 [*]
 * * 3. 블루투스 모드 변경 (BLE 전용 -> Classic 포함):
 * -> [Bluetooth controller] 
 * -> [Bluetooth mode] 
 * -> 'Bluetooth Dual Mode' 또는 'BR/EDR Only' 선택
 * * 4. Bluedroid 및 SPP(Serial Port Profile) 활성화:
 * -> 뒤로 가기(Esc) 후 [Bluedroid Enable] 메뉴로 이동
 * -> 'Classic Bluetooth' 항목 스페이스바를 눌러 체크 [*]
 * -> 하위에 나타나는 'SPP' 항목 스페이스바를 눌러 체크 [*]
 * * 5. 저장 및 강력한 재빌드 (기존 캐시 제거 필수):
 * -> 'S' 키를 눌러 저장 후, 'Q' 키를 눌러 종료
 * -> Full Clean(쓰레기통 아이콘), Build(스패너 아이콘)
 * ====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "uil_DRV8833.h"

// ==============================================================================
// 하드웨어 핀 맵핑 (DRV8833 모터 드라이버 및 서보 모터 제어용)
// ==============================================================================

// 구동용 DC 모터(MOTOR_1) 제어 핀
#define MOTOR_IN1_PIN       27
#define MOTOR_IN2_PIN       26
// 조향용 서보 모터(MOTOR_2) 제어 핀
#define SERVO_MOTOR_IN3_PIN 25
#define SERVO_MOTOR_IN4_PIN 33

#define STBY_PIN            14

// ==============================================================================
// 전역 제어 상태 및 통신 버퍼 변수
// ==============================================================================
// 모터 구동의 목표 속도를 저장하는 변수 (기어비 및 가속/감속 로직에 의해 결정됨)
static float target_speed = 0.0;

// 안드로이드 앱에서 전송하는 블루투스 통신 데이터를 임시 저장하는 수신 버퍼
#define RX_BUF_SIZE 128
static char rx_buf[RX_BUF_SIZE];
static int rx_idx = 0;

// 함수 사전 선언
static void init_hardware(void);
static void parse_packet(const char* packet);
static void set_servo_angle(int physical_angle);
static void motor_scurve_task(void *pvParameter);
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

// ==============================================================================
// 하드웨어 초기화 (DRV8833 라이브러리 설정)
// ==============================================================================
static void init_hardware(void) {
    // 직접 제작한 DRV8833 모터 드라이버 라이브러리 초기화: 
    // MOTOR_1은 구동용, MOTOR_2는 조향용(서보)으로 핀 매핑
    DRV8833_Init_Default(MOTOR_1, MOTOR_IN1_PIN, MOTOR_IN2_PIN);
    DRV8833_Init_Default(MOTOR_2, SERVO_MOTOR_IN3_PIN, SERVO_MOTOR_IN4_PIN);

    /*
     * 서보 모터 제어를 위해 MOTOR_2의 기본 회전 방향을 정방향(CW)으로 설정합니다.
     * 기준: 정방향(Forward) 작동 시 좌회전, 역방향(Backward) 작동 시 우회전
     * 실제 기구 연결에 따라 좌/우 방향이 반대로 동작할 경우,
     * 아래 함수에서 ROTATION_CW 대신 ROTATION_CCW로 변경하여 방향을 교정할 수 있습니다.
     */
    DRV8833_SetRotation(MOTOR_2, ROTATION_CW);

    DRV8833_Enable(STBY_PIN); // 드라이버 활성화

    // 구동 시작 시 초기 조향 각도를 0(중앙)으로 설정
    set_servo_angle(0);
}

// ==============================================================================
// 조향(서보) 모터 제어
// 안드로이드 앱으로부터 전달받은 조향 각도(-40~40)를 바탕으로 좌/우 회전 수행
// ==============================================================================
static void set_servo_angle(int physical_angle) {
    // 하드웨어 한계를 고려하여 각도를 -40도 ~ 40도로 제한(Clamping)
    if (physical_angle < -40) physical_angle = -40;
    if (physical_angle > 40) physical_angle = 40;

    // 조향 모터에 가할 PWM 전력 (0~255 범위 중 절반 수준의 파워 사용)
    // 서보 모터 조향 속도를 해당 변수에서 조절하세요.(초기 50%, servo_speed 범위 : 0~255)
    int servo_speed = 255 / 2;    // half power

    // 각도에 따른 모터 회전 방향 설정:
    // 데드존(-10 ~ 10)을 두어 약간의 흔들림에 모터가 민감하게 반응하지 않도록 처리
    if (physical_angle < -5)
    {
        // 각도가 -10 미만일 경우 왼쪽으로 회전 (정방향 구동)
        DRV8833_SetDirection(MOTOR_2, DIRECTION_FORWARD);   // LEFT
    }
    else if (physical_angle > 5)
    {
        // 각도가 10 초과일 경우 오른쪽으로 회전 (역방향 구동)
        DRV8833_SetDirection(MOTOR_2, DIRECTION_BACKWARD);   // RIGHT
    }
    else
    {
        // 각도가 데드존 이내인 경우 모터 파워 차단 (직진 상태 유지)
        servo_speed = 0;
    }

    // 결정된 모터 속도(PWM) 및 방향을 DRV8833에 적용
    DRV8833_SetSpeed(MOTOR_2, servo_speed);
}

// ==============================================================================
// 구동 모터 가속/감속 제어 태스크 (S-Curve 적용)
// 급가속 및 급정거로 인한 모터/드라이버의 무리를 방지하기 위해 소프트 제어 수행
// ==============================================================================
static void motor_scurve_task(void *pvParameter) {
    // 2단계 저주파 통과 필터(Low-pass filter) 변수 및 계수 설정
    float filter1 = 0.0, filter2 = 0.0;
    const float alpha = 0.08; 

    while (1) {
        // 목표 속도(target_speed)에 대해 부드러운 전환 효과(S-Curve)를 위한 필터링 연산
        filter1 = filter1 + alpha * (target_speed - filter1);
        filter2 = filter2 + alpha * (filter1 - filter2);

        // 필터링된 결과값(0~100)을 실제 모터의 PWM 해상도(0~255)에 맞게 변환
        int current_pwm = (int)(fabs(filter2) * 2.55);
        if (current_pwm > 255) current_pwm = 255;

        // 필터값의 부호를 판별하여 모터의 구동 방향 결정 (기어 상태 자동 반영됨)
        // 데드존(-2.0 ~ 2.0)을 두어 정지 상태에서의 미세 떨림/구동 방지
        if (filter2 > 2.0) { 
            DRV8833_SetDirection(MOTOR_1, DIRECTION_FORWARD);
        } else if (filter2 < -2.0) { 
            DRV8833_SetDirection(MOTOR_1, DIRECTION_BACKWARD);
        } else { 
            current_pwm = 0;
        }

        // 최종 계산된 PWM 값을 DRV8833 구동 모터에 인가
        DRV8833_SetSpeed(MOTOR_1, current_pwm);
        
        // 태스크 지연 (10ms 주기로 제어 루프 반복)
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// ==============================================================================
// 안드로이드 앱 통신 패킷 파싱
// S(조향), G(기어), A(가속), B(브레이크) 값을 분석하여 차량의 움직임 제어 반영
// ==============================================================================
static void parse_packet(const char* packet) {
    int parsed_angle = 0;  // 조향 각도 (-40 ~ 40)
    int gear = 0;          // 기어 상태 (1: 전진, -1: 후진, 0: 중립/주차)
    int accel = 0;         // 가속 페달 값 (0 ~ 100)
    int brake = 0;         // 브레이크 페달 값 (0 ~ 100)

    // 앱에서 보내는 문자열 "S:{각도},G:{기어},A:{가속},B:{브레이크}" 형식을 파싱
    // 인자가 총 4개이므로 정상적으로 추출되었을 경우 4를 반환
    int parsed_count = sscanf(packet, "S:%d,G:%d,A:%d,B:%d", &parsed_angle, &gear, &accel, &brake);

    // 1. 조향 제어 (주차 상태여도 서보 모터는 핸들링을 위해 독립적으로 동작 가능)
    if (parsed_count >= 1) {
        set_servo_angle(parsed_angle);
    }

    // 2. 구동 제어 (4개의 파라미터를 모두 성공적으로 수신했을 때 실행)
    if (parsed_count == 4) {
        // 앱에서 전달된 입력값이 설정 범위를 벗어나지 않도록 클램핑 처리
        if (accel > 100) accel = 100; else if (accel < 0) accel = 0;
        if (brake > 100) brake = 100; else if (brake < 0) brake = 0;
        
        // 순수 동력 계산: 가속 값에서 브레이크 값을 차감
        // 브레이크를 많이 밟아도 역방향 구동으로 이어지지 않도록 0으로 하한선 적용
        int net_power = accel - brake;
        if (net_power < 0) net_power = 0;

        // ⭐️ 핵심 로직: 
        // 도출된 순수 동력에 기어 값(전진=1, 중립=0, 후진=-1)을 곱하여
        // 차량의 구동 방향과 목표 속도를 타겟 스피드(target_speed) 변수에 한번에 설정
        target_speed = (float)(net_power * gear);
    }
}

// ==============================================================================
// Bluetooth Classic SPP 콜백 함수
// 안드로이드 앱과의 블루투스 SPP 연결 상태, 이벤트 처리 및 데이터 수신 담당
// ==============================================================================
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            // SPP 프로파일 초기화 완료 시 서버 소켓 시작
            esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
            break;
        case ESP_SPP_START_EVT:
            // 안드로이드 앱에서 검색될 블루투스 디바이스 이름 설정 및 스캔 활성화
            esp_bt_gap_set_device_name("ESP32_RC_CAR");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            break;
        case ESP_SPP_DATA_IND_EVT:
            // 안드로이드 앱으로부터 시리얼 데이터 수신 이벤트 발생 시
            for(int i = 0; i < param->data_ind.len; i++) {
                char c = param->data_ind.data[i];
                // 개행문자('\n' 또는 '\r')를 한 패킷의 끝으로 인식하여 분리 처리
                if (c == '\n' || c == '\r') {
                    if (rx_idx > 0) {
                        rx_buf[rx_idx] = '\0';        // 문자열의 끝에 NULL 추가
                        printf("%s\n", rx_buf);       // 수신 데이터 콘솔 로그 출력
                        parse_packet(rx_buf);         // 수신된 문자열 패킷 분석 및 모터 제어 반영
                        rx_idx = 0;                   // 다음 데이터를 위해 버퍼 인덱스 초기화
                    }
                } else {
                    // 수신된 문자를 버퍼에 저장 (버퍼 오버플로우 방지 로직 포함)
                    if (rx_idx < RX_BUF_SIZE - 1) rx_buf[rx_idx++] = c;
                }
            }
            break;
        case ESP_SPP_CLOSE_EVT:
            // 앱과의 블루투스 연결이 의도치 않게 끊어졌을 때의 안전(Fail-Safe) 로직
            target_speed = 0.0; // 즉각적으로 차량 목표 속도를 0으로 만들어 자동 정지
            DRV8833_Disable();  // 드라이버 비활성화
            break;
        default: break;
    }
}

// ==============================================================================
// 메인 어플리케이션(진입점)
// ==============================================================================
void app_main(void) {
    // 1. 비휘발성 저장소(NVS) 플래시 초기화 (블루투스 기기 정보 등 내부 설정 보관 목적)
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. 모터 제어를 위한 하드웨어 핀 및 직접 제작한 DRV8833 드라이버 초기화
    init_hardware();
    
    // 3. 부드러운 가감속(S-Curve) 처리를 위한 모터 제어 태스크 백그라운드 실행
    xTaskCreate(motor_scurve_task, "motor_scurve_task", 2048, NULL, 5, NULL);

    // 4. Bluetooth Classic 모드 사용 설정 및 스택 초기화
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE)); // 사용하지 않는 BLE 메모리 해제
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)); // Classic 블루투스 모드 활성화
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    
    // 5. SPP 콜백 함수 등록 및 Bluetooth SPP 모듈 가동
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_init(ESP_SPP_MODE_CB));
}