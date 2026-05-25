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

// --- 핀 맵핑 (L298N & Servo) ---
#define PIN_MOTOR_L_IN1 27
#define PIN_MOTOR_L_IN2 26
#define PIN_MOTOR_R_IN3 25
#define PIN_MOTOR_R_IN4 33
#define PIN_MOTOR_ENA   14
#define PIN_MOTOR_ENB   32
#define PIN_SERVO       13

// --- 제어 상태 변수 ---
static float target_speed = 0.0;
#define RX_BUF_SIZE 128
static char rx_buf[RX_BUF_SIZE];
static int rx_idx = 0;

static void init_hardware(void);
static void parse_packet(const char* packet);
static void set_servo_angle(int physical_angle);
static void motor_scurve_task(void *pvParameter);
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

// ---------------------------------------------------------
// 하드웨어 초기화
// ---------------------------------------------------------
static void init_hardware(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<PIN_MOTOR_L_IN1) | (1ULL<<PIN_MOTOR_L_IN2) | 
                        (1ULL<<PIN_MOTOR_R_IN3) | (1ULL<<PIN_MOTOR_R_IN4),
        .pull_down_en = 0, .pull_up_en = 0
    };
    gpio_config(&io_conf);

    // 구동 모터 PWM
    ledc_timer_config_t dc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&dc_timer);

    ledc_channel_config_t ena_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_MOTOR_ENA, .duty = 0, .hpoint = 0
    };
    ledc_channel_config_t enb_ch = ena_ch;
    enb_ch.channel = LEDC_CHANNEL_1; enb_ch.gpio_num = PIN_MOTOR_ENB;
    ledc_channel_config(&ena_ch);
    ledc_channel_config(&enb_ch);

    // 서보 모터 PWM (50Hz)
    ledc_timer_config_t servo_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_14_BIT, .freq_hz = 50, .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&servo_timer);

    ledc_channel_config_t servo_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_2,
        .timer_sel = LEDC_TIMER_1, .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_SERVO, .duty = 0, .hpoint = 0
    };
    ledc_channel_config(&servo_ch);

    set_servo_angle(0);
}

// ---------------------------------------------------------
// 액추에이터 제어
// ---------------------------------------------------------
static void set_servo_angle(int physical_angle) {
    if (physical_angle < -40) physical_angle = -40;
    if (physical_angle > 40) physical_angle = 40;

    int pulse_us = 1500 + (physical_angle * 1000 / 90);
    uint32_t duty = (pulse_us * 16384) / 20000;
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

static void motor_scurve_task(void *pvParameter) {
    float filter1 = 0.0, filter2 = 0.0;
    const float alpha = 0.08; 

    while (1) {
        filter1 = filter1 + alpha * (target_speed - filter1);
        filter2 = filter2 + alpha * (filter1 - filter2);

        int current_pwm = (int)(fabs(filter2) * 2.55);
        if (current_pwm > 255) current_pwm = 255;

        // target_speed의 부호(기어 상태)에 따라 모터 방향이 자연스럽게 결정됩니다.
        if (filter2 > 2.0) { 
            gpio_set_level(PIN_MOTOR_L_IN1, 1); gpio_set_level(PIN_MOTOR_L_IN2, 0);
            gpio_set_level(PIN_MOTOR_R_IN3, 0); gpio_set_level(PIN_MOTOR_R_IN4, 1);
        } else if (filter2 < -2.0) { 
            gpio_set_level(PIN_MOTOR_L_IN1, 0); gpio_set_level(PIN_MOTOR_L_IN2, 1);
            gpio_set_level(PIN_MOTOR_R_IN3, 1); gpio_set_level(PIN_MOTOR_R_IN4, 0);
        } else { 
            gpio_set_level(PIN_MOTOR_L_IN1, 0); gpio_set_level(PIN_MOTOR_L_IN2, 0);
            gpio_set_level(PIN_MOTOR_R_IN3, 0); gpio_set_level(PIN_MOTOR_R_IN4, 0);
            current_pwm = 0;
        }

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, current_pwm);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, current_pwm);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// ---------------------------------------------------------
// 통신 패킷 파싱 (⭐️ 기어 로직 완벽 적용)
// ---------------------------------------------------------
static void parse_packet(const char* packet) {
    int parsed_angle = 0;
    int gear = 0;
    int accel = 0;
    int brake = 0;

    // 인자가 S, G, A, B 4개이므로 성공 시 4를 반환합니다.
    int parsed_count = sscanf(packet, "S:%d,G:%d,A:%d,B:%d", &parsed_angle, &gear, &accel, &brake);

    // 1. 조향 제어 (주차 상태여도 서보모터는 독립적으로 동작)
    if (parsed_count >= 1) {
        set_servo_angle(parsed_angle);
    }

    // 2. 구동 제어 (파싱 카운트 5 -> 4로 수정)
    if (parsed_count == 4) {
        // 입력값 클램핑
        if (accel > 100) accel = 100; else if (accel < 0) accel = 0;
        if (brake > 100) brake = 100; else if (brake < 0) brake = 0;
        
        // 순수 동력 계산 (브레이크를 너무 밟았다고 후진하지 않도록 0으로 하한선 적용)
        int net_power = accel - brake;
        if (net_power < 0) net_power = 0;

        // ⭐️ 핵심: 동력에 기어 값(1, 0, -1)을 곱해서 타겟 스피드의 방향과 정지를 한 번에 결정
        target_speed = (float)(net_power * gear);
    }
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
            break;
        case ESP_SPP_START_EVT:
            esp_bt_gap_set_device_name("ESP32_RC_CAR");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            break;
        case ESP_SPP_DATA_IND_EVT:
            for(int i = 0; i < param->data_ind.len; i++) {
                char c = param->data_ind.data[i];
                if (c == '\n' || c == '\r') {
                    if (rx_idx > 0) {
                        rx_buf[rx_idx] = '\0';
                        printf("%s\n", rx_buf);
                        parse_packet(rx_buf);
                        rx_idx = 0;
                    }
                } else {
                    if (rx_idx < RX_BUF_SIZE - 1) rx_buf[rx_idx++] = c;
                }
            }
            break;
        case ESP_SPP_CLOSE_EVT:
            target_speed = 0.0; // 연결 끊기면 자동 정지
            break;
        default: break;
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    
    init_hardware();
    xTaskCreate(motor_scurve_task, "motor_scurve_task", 2048, NULL, 5, NULL);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_init(ESP_SPP_MODE_CB));
}