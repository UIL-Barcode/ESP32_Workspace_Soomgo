#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

// ============================================================================
// [1] 드라이버 영역 (나중에 pwm_c 폴더로 옮길 부분)
// ============================================================================

// 파트너님의 철학(하드코딩 배제)을 유지하기 위한 설정 구조체
typedef struct {
    int gpio_pin;
    ledc_channel_t channel;
    ledc_timer_t timer;
    uint32_t frequency;
    ledc_timer_bit_t resolution;
} pwm_config_t;

// PWM 초기화 함수
esp_err_t init_pwm(const pwm_config_t *config) {
    // 1. 타이머 설정
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = config->resolution,
        .timer_num        = config->timer,
        .freq_hz          = config->frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) return ret;

    // 2. 채널 설정 (핀 연결)
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = config->gpio_pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = config->channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = config->timer,
        .duty           = 0,
        .hpoint         = 0
    };
    return ledc_channel_config(&ledc_channel);
}

// 듀티비(밝기/속도) 제어 함수
void set_pwm_duty(ledc_channel_t channel, uint32_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

// ============================================================================
// [2] 메인 어플리케이션 영역 (실제 프로젝트에서 사용할 부분)
// ============================================================================

void app_main(void) {
    // 사용할 PWM 스펙 정의 (GPIO 18번 사용, 5kHz, 13비트 분해능)
    pwm_config_t my_pwm = {
        .gpio_pin = 18,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency = 5000,               
        .resolution = LEDC_TIMER_13_BIT  // 13비트: 0 ~ 8191 범위
    };

    // 초기화 실행
    if (init_pwm(&my_pwm) == ESP_OK) {
        printf("PWM Initialized on GPIO %d\n", my_pwm.gpio_pin);
    } else {
        printf("PWM Initialization Failed!\n");
        return;
    }

    uint32_t duty = 0;
    int fade_amount = 100; // 한 번에 변할 듀티비 크기

    // 무한 루프: LED가 서서히 밝아졌다가 어두워집니다.
    while (1) {
        set_pwm_duty(my_pwm.channel, duty);

        // 듀티비 계산
        duty += fade_amount;
        
        // 13비트 최대값(8191) 근처나 0에 도달하면 방향을 반대로 바꿈
        if (duty >= 8191 || duty <= 0) {
            fade_amount = -fade_amount; 
        }

        // 20ms 대기 (부드러운 변화를 위함)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}