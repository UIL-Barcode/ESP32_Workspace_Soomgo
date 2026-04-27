#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h" // ESP-IDF v5.x 최신 ADC API

// ============================================================================
// [1] 하드웨어 핀 및 설정값 정의
// ============================================================================
// 조이스틱 설정 (ADC1 채널 6은 보통 GPIO 34번 핀입니다)
#define JOYSTICK_X_ADC_CHAN ADC_CHANNEL_6 // GPIO34
#define JOYSTICK_Y_ADC_CHAN ADC_CHANNEL_7 // GPIO35

// L298N 모터 드라이버 핀 설정
#define ENA_PWM_PIN         14  // 속도 제어 (PWM)
#define IN1_PIN             26  // 방향 1
#define IN2_PIN             27  // 방향 2

// PWM 스펙 (13비트: 0 ~ 8191)
#define PWM_MAX_DUTY        8191
#define JOYSTICK_DEADZONE   200 // 조이스틱 중앙 떨림 방지용 데드존

// ============================================================================
// [2] 초기화 함수들
// ============================================================================
adc_oneshot_unit_handle_t adc1_handle;

void init_joystick() {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // 0 ~ 4095
        .atten = ADC_ATTEN_DB_12,    // 3.3V 전체 범위 읽기
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_X_ADC_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_Y_ADC_CHAN, &config));
}

void init_motor() {
    // IN1, IN2 핀을 일반 출력(Output) 모드로 설정
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IN1_PIN) | (1ULL << IN2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // ENA 핀을 위한 PWM(LEDC) 설정
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000, // 5kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = ENA_PWM_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// 모터 방향 및 속도 적용 함수
void set_motor_output(int duty) {
    if (duty > 0) {
        // 정방향
        gpio_set_level(IN1_PIN, 1);
        gpio_set_level(IN2_PIN, 0);
    } else if (duty < 0) {
        // 역방향
        gpio_set_level(IN1_PIN, 0);
        gpio_set_level(IN2_PIN, 1);
        duty = -duty; // PWM은 양수여야 하므로 절대값 처리
    } else {
        // 정지
        gpio_set_level(IN1_PIN, 0);
        gpio_set_level(IN2_PIN, 0);
    }
    
    // PWM 값 업데이트
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// ============================================================================
// [3] 메인 어플리케이션 (S-Curve 적용)
// ============================================================================
void app_main(void) {
    init_joystick();
    init_motor();

    printf("Joystick Motor Control with S-Curve Started!\n");

    // S-Curve를 위한 필터 변수들
    float target_speed = 0.0f;
    float filter1_out = 0.0f;
    float current_speed = 0.0f;
    
    // 이 값이 작을수록 가감속이 부드러워지고(S-Curve가 길어짐), 클수록 민첩해집니다. (0.0 ~ 1.0)
    float ALPHA = 0.1f; 

    while (1) {
        // 1. 조이스틱 값 읽기 (0 ~ 4095)
        int adc_raw = 0;
        adc_oneshot_read(adc1_handle, JOYSTICK_X_ADC_CHAN, &adc_raw);

        // 2. 조이스틱 값을 모터 속도(-8191 ~ 8191)로 매핑
        // 중앙값(약 2048)을 0으로 맞춥니다.
        int centered_val = adc_raw - 2048; 
        
        // 데드존 처리 (조이스틱 중앙의 미세한 흔들림 무시)
        if (abs(centered_val) < JOYSTICK_DEADZONE) {
            centered_val = 0;

            int adc_raw_y = 0;
            adc_oneshot_read(adc1_handle, JOYSTICK_Y_ADC_CHAN, &adc_raw_y);
            if (abs(adc_raw_y - 2048) > JOYSTICK_DEADZONE)
            {
                if ((ALPHA > 0.01f) && (adc_raw_y < 1000))   ALPHA -= 0.01f;
                if ((ALPHA < 1.0f) && (adc_raw_y > 3000))   ALPHA += 0.01f;

                if (ALPHA < 0.0001f)   ALPHA = 0.01f;
                if (ALPHA > 1.0000f)   ALPHA = 1.0f;
            }
        }

        // -2048~2047 범위를 -8191~8191 범위로 확장
        target_speed = (float)centered_val * (PWM_MAX_DUTY / 2048.0f);

        // 한계치 제한 (Clamp)
        if (target_speed > PWM_MAX_DUTY) target_speed = PWM_MAX_DUTY;
        if (target_speed < -PWM_MAX_DUTY) target_speed = -PWM_MAX_DUTY;

        // ==============================================================
        // ⭐️ 핵심: 2차 EMA 필터를 이용한 동적 S-Curve 생성 ⭐️
        // ==============================================================
        // 첫 번째 필터 (선형 가감속의 느낌)
        filter1_out += ALPHA * (target_speed - filter1_out);
        
        // 두 번째 필터 (모서리를 둥글게 깎아 완벽한 S-Curve 완성)
        current_speed += ALPHA * (filter1_out - current_speed);

        // 3. 모터에 최종 속도 전달
        set_motor_output((int)current_speed);

        // 디버깅용 출력 (Serial Plotter로 보면 예쁜 S-Curve를 볼 수 있습니다)
        printf("Target:%4d, Current:%4d, ALPHA:%.2f\n", (int)target_speed, (int)current_speed, ALPHA);
    
        // 10ms 주기 실행 (제어 루프 주기)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}