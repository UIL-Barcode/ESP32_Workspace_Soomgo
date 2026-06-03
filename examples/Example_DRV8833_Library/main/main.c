#include <stdio.h>
#include "DRV8833/uil_DRV8833.h"

// 핀 번호 정의 (ESP32 기준)
#define MOTOR_IN1_PIN 26  // 방향 제어 1
#define MOTOR_IN2_PIN 27  // 방향 제어 2

void app_main(void) {
    DRV8833_Init_Default(MOTOR_1, MOTOR_IN1_PIN, MOTOR_IN2_PIN);

    // 4. 메인 제어 루프
    while (1) {
        DRV8833_SetDirection(MOTOR_1, DIRECTION_FORWARD);

        // 1초 동안 속도(Duty)를 0에서 255까지 점진적 증가
        for (int duty = 0; duty <= 255; duty++) {
            DRV8833_SetSpeed(MOTOR_1, duty);
            // 256번 반복 * 4ms 지연 = 약 1024ms (약 1초간 가속)
            vTaskDelay(pdMS_TO_TICKS(4)); 
        }

        // 방향 전환 시 모터 보호를 위해 잠시 정지 (0.5초)
        DRV8833_SetSpeed(MOTOR_1, 0);
        vTaskDelay(pdMS_TO_TICKS(500));

        // --- [후진 모드] ---
        DRV8833_SetDirection(MOTOR_1, DIRECTION_BACKWARD);
        
        // 1초 동안 속도(Duty)를 0에서 255까지 점진적 증가
        for (int duty = 0; duty <= 255; duty++) {
            DRV8833_SetSpeed(MOTOR_1, duty);
            vTaskDelay(pdMS_TO_TICKS(4)); 
        }

        // 다시 방향 전환 전 모터 정지 (0.5초)
        DRV8833_SetSpeed(MOTOR_1, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}