#pragma once

#ifndef UIL_DRV8833_H
#define UIL_DRV8833_H

#include <stdbool.h>

/**
 * @brief 모터의 기본 물리적 회전(설치) 방향 설정
 */
typedef enum {
    ROTATION_CW = 1,  // 시계 방향 (정방향)
    ROTATION_CCW = 2  // 반시계 방향 (역방향)
} DEFAULT_ROTATION;

/**
 * @brief 모터의 현재 제어 구동 방향 (전진/후진)
 */
typedef enum {
    DIRECTION_FORWARD = 1,  // 전진
    DIRECTION_BACKWARD = 2  // 후진
} DIRECTION;

/**
 * @brief 제어할 모터 식별자
 */
typedef enum {
    MOTOR_1 = 1,  // 첫 번째 모터
    MOTOR_2 = 2   // 두 번째 모터
} MOTOR;

/**
 * @brief DRV8833 모터 제어기 설정 구조체
 */
typedef struct {
    DEFAULT_ROTATION rotation;   // 모터의 기본 회전 방향 (CW/CCW)
    DIRECTION direction;         // 현재 구동 방향 (전진/후진)
    int in_pin1;                 // PWM 제어 입력 핀 1 (IN1 또는 IN3)
    int in_pin2;                 // PWM 제어 입력 핀 2 (IN2 또는 IN4)
    int speed;                   // 현재 속도 (0-255 범위의 PWM 듀티 사이클)
} DRV8833_config_t;

// 모터 1, 2의 전역 설정 인스턴스 변수
extern DRV8833_config_t DRV8833_M1;
extern DRV8833_config_t DRV8833_M2;

// 드라이버 준비(활성화) 상태
extern bool is_ready;

/**
 * @brief DRV8833 모터 드라이버를 활성화하고 대기 모드(Standby)를 해제합니다.
 * 
 * @param pin DRV8833의 STBY(Standby) 핀과 연결된 ESP32의 GPIO 핀 번호
 */
void DRV8833_Enable(int pin);

/**
 * @brief DRV8833 모터 드라이버를 비활성화하여 대기 모드(Standby)로 진입합니다.
 */
void DRV8833_Disable(void);

/**
 * @brief 두 개의 핀 번호를 입력받아 기본 모터 설정 구조체를 생성하여 반환합니다.
 * 
 * @param pin1 모터 제어용 입력 핀 1
 * @param pin2 모터 제어용 입력 핀 2
 * @return DRV8833_config_t 초기화된 기본 설정 구조체
 */
DRV8833_config_t DRV8833_GetDefault(int pin1, int pin2);

/**
 * @brief 주어진 설정 구조체를 사용하여 특정 모터(M1 또는 M2)의 설정을 갱신(초기화)합니다.
 * 
 * @param motor 대상 모터 식별자 (MOTOR_1 또는 MOTOR_2)
 * @param config 갱신할 모터 설정 정보가 담긴 구조체 포인터
 */
void DRV8833_InitMotor(MOTOR motor, DRV8833_config_t* config);

/**
 * @brief 모터 1과 모터 2의 설정을 한 번에 같이 초기화합니다.
 * 
 * @param config1 모터 1에 적용할 설정 구조체 포인터
 * @param config2 모터 2에 적용할 설정 구조체 포인터
 */
void DRV8833_Init(DRV8833_config_t* config1, DRV8833_config_t* config2);

/**
 * @brief 기본 설정값으로 모터를 초기화하고, 속도 제어에 필요한 ESP32의 LEDC(PWM) 하드웨어 타이머와 채널을 설정합니다.
 * 
 * @param motor 설정할 대상 모터 (MOTOR_1 또는 MOTOR_2)
 * @param pin1 모터 제어용 입력 핀 1
 * @param pin2 모터 제어용 입력 핀 2
 */
void DRV8833_Init_Default(MOTOR motor, int pin1, int pin2);

/**
 * @brief 모터가 실제로 장착된 회전 방향(CW/CCW)을 설정하여, 논리적 전/후진이 물리적 구동과 일치하도록 합니다.
 * 
 * @param motor 대상 모터 식별자
 * @param rotation 회전 방향 (ROTATION_CW 또는 ROTATION_CCW)
 */
void DRV8833_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation);

/**
 * @brief 모터의 구동 방향(전진/후진)을 설정하고, 장착 방향(CW/CCW)을 고려하여 실제 핀의 PWM 출력을 업데이트합니다.
 * 
 * @param motor 대상 모터 식별자
 * @param direction 구동 방향 (DIRECTION_FORWARD 또는 DIRECTION_BACKWARD)
 */
void DRV8833_SetDirection(MOTOR motor, DIRECTION direction);

/**
 * @brief 하위 레벨 함수로, 특정 모터의 개별 핀(1번 또는 2번)에 대한 PWM 듀티 사이클을 직접 설정합니다.
 * 
 * @param motor 대상 모터 식별자
 * @param pin 제어할 모터의 핀 번호 (1 또는 2)
 * @param speed 적용할 속도값 (0~255)
 */
void DRV8833_PWM_SetSpeed(MOTOR motor, int pin, int speed);

/**
 * @brief 모터의 전체 동작 속도를 갱신하고 현재 설정된 구동 방향에 맞춰 바로 속도를 적용합니다.
 * 
 * @param motor 대상 모터 식별자
 * @param speed 목표 속도값 (0~255)
 */
void DRV8833_SetSpeed(MOTOR motor, int speed);

#endif