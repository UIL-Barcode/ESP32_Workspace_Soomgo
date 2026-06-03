#pragma once

#ifndef UIL_DRV8833_H
#define UIL_DRV8833_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

typedef enum {
    ROTATION_CW = 1,
    ROTATION_CCW = 2
} DEFAULT_ROTATION;

typedef enum {
    DIRECTION_FORWARD = 1,
    DIRECTION_BACKWARD = 2
} DIRECTION;

typedef enum {
    MOTOR_1 = 1,
    MOTOR_2 = 2
} MOTOR;

typedef struct {
    DEFAULT_ROTATION rotation; // 모터의 기본 회전 방향
    DIRECTION current_direction; // 현재 회전 방향
    int in_pin1; // PWM 제어 핀 1
    int in_pin2; // PMW 제어 핀 2
    int speed; // 현재 속도 (0-255)
} DRV8833_config_t;

extern DRV8833_config_t DRV8833_M1;
extern DRV8833_config_t DRV8833_M2;

DRV8833_config_t DRV8833_GetDefault(int pin1, int pin2);
void DRV8833_InitMotor(MOTOR motor, DRV8833_config_t* config);
void DRV8833_Init(DRV8833_config_t* config1, DRV8833_config_t* config2);
void DRV8833_Init_Default(MOTOR motor, int pin1, int pin2);
void DRV8833_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation);
void DRV8833_SetDirection(MOTOR motor, DIRECTION direction);
void DRV8833_PWM_SetSpeed(MOTOR motor, int pin, int speed);
void DRV8833_SetSpeed(MOTOR motor, int speed);

#endif