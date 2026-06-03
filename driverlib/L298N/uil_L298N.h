#pragma once

#ifndef UIL_L298N_H
#define UIL_L298N_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
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
    int en_pin; // PWM 제어 핀
    int in_pin1; // 방향 제어 핀 1
    int in_pin2; // 방향 제어 핀 2
    int speed; // 현재 속도 (0-255)
} L298N_Config;


extern L298N_Config L298N_M1;
extern L298N_Config L298N_M2;

L298N_Config L298N_GetDefault(int en, int pin1, int pin2);
void L298N_InitMotor(MOTOR motor, L298N_Config* config);
void L298N_Init(L298N_Config* config1, L298N_Config* config2);
void L298N_Init_Default(MOTOR motor, int en, int pin1, int pin2);
void L298N_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation);
void L298N_SetDirection(MOTOR motor, DIRECTION direction);
void L298N_SetSpeed(MOTOR motor, int speed);

#endif