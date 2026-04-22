#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// 핀 번호 정의 (ESP32 기준)
#define MOTOR_ENA_PIN 14  // 모터 속도 제어 (PWM)
#define MOTOR_IN1_PIN 26  // 방향 제어 1
#define MOTOR_IN2_PIN 27  // 방향 제어 2

void app_main(void) {
    // 1. 방향 제어용 GPIO 초기화 (IN1, IN2)
    gpio_reset_pin(MOTOR_IN1_PIN);
    gpio_reset_pin(MOTOR_IN2_PIN);
    gpio_set_direction(MOTOR_IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_IN2_PIN, GPIO_MODE_OUTPUT);

    // 2. 속도 제어용 PWM(LEDC) 타이머 설정
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT, // 8비트 해상도 (0 ~ 255)
        .freq_hz          = 1000,             // PWM 주파수: 1kHz (DC모터 범용)
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 3. PWM 채널 설정 및 핀 연결
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_ENA_PIN,
        .duty           = 0, // 초기 속도 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // 4. 메인 제어 루프
    while (1) {
        // --- [전진 모드] ---
        gpio_set_level(MOTOR_IN1_PIN, 1);
        gpio_set_level(MOTOR_IN2_PIN, 0);
        
        // 1초 동안 속도(Duty)를 0에서 255까지 점진적 증가
        for (int duty = 0; duty <= 255; duty++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            
            // 256번 반복 * 4ms 지연 = 약 1024ms (약 1초간 가속)
            vTaskDelay(pdMS_TO_TICKS(4)); 
        }

        // 방향 전환 시 모터 보호를 위해 잠시 정지 (0.5초)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(500));

        // --- [후진 모드] ---
        gpio_set_level(MOTOR_IN1_PIN, 0);
        gpio_set_level(MOTOR_IN2_PIN, 1);
        
        // 1초 동안 속도(Duty)를 0에서 255까지 점진적 증가
        for (int duty = 0; duty <= 255; duty++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            
            vTaskDelay(pdMS_TO_TICKS(4)); 
        }

        // 다시 방향 전환 전 모터 정지 (0.5초)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}