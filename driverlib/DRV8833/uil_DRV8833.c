#include "uil_DRV8833.h"
#include <string.h>
#include <assert.h>

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

DRV8833_config_t DRV8833_M1;
DRV8833_config_t DRV8833_M2;

DRV8833_config_t DRV8833_GetDefault(int pin1, int pin2)
{
    DRV8833_config_t default_config = {
        ROTATION_CW,
        DIRECTION_FORWARD,
        pin1,
        pin2,
        0
    };
    return default_config;
}

void DRV8833_InitMotor(MOTOR motor, DRV8833_config_t* config)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    (void)memcpy(target, config, sizeof(DRV8833_config_t));
}

void DRV8833_Init(DRV8833_config_t* config1, DRV8833_config_t* config2)
{
    DRV8833_InitMotor(MOTOR_1, config1);
    DRV8833_InitMotor(MOTOR_2, config2);
}

void DRV8833_Init_Default(MOTOR motor, int pin1, int pin2)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);

    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;
    DRV8833_config_t default_config = DRV8833_GetDefault(pin1, pin2);
    (void)memcpy(target, &default_config, sizeof(DRV8833_config_t));
    
    // 2. 속도 제어용 PWM(LEDC) 타이머 설정
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT, // 8비트 해상도 (0 ~ 255)
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 3. PWM 채널 설정 및 핀 연결
    ledc_channel_config_t ledc_channel_0 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (motor == MOTOR_1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = target->in_pin1,
        .duty           = 0, // 초기 속도 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_0);

    // 3. PWM 채널 설정 및 핀 연결
    ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (motor == MOTOR_1) ? LEDC_CHANNEL_1 : LEDC_CHANNEL_3,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = target->in_pin2,
        .duty           = 0, // 초기 속도 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_1);
}

void DRV8833_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    target->rotation = rotation;
}

void DRV8833_SetDirection(MOTOR motor, DIRECTION direction)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;
    // pin_p : motor input pin with positive.
    // pin_n : motor input pin with negative.
    int pin_p = 0, pin_n = 0;

    // when motor insatlled cw way. -> 1 ~ 2.
    // when motor installed ccw way. -> 2 ~ 1.
    pin_p = (target->rotation == ROTATION_CW) ? 1 : 2;
    pin_n = (target->rotation == ROTATION_CW) ? 2 : 1;

    switch(direction)
    {
        case DIRECTION_FORWARD:
        {
            DRV8833_PWM_SetSpeed(motor, pin_n, 0);
            DRV8833_PWM_SetSpeed(motor, pin_p, target->speed);
        }
        break;
        case DIRECTION_BACKWARD:
        {
            DRV8833_PWM_SetSpeed(motor, pin_p, 0);
            DRV8833_PWM_SetSpeed(motor, pin_n, target->speed);
        }
        break;
        default:
        {
            DRV8833_PWM_SetSpeed(motor, 1, 0);
            DRV8833_PWM_SetSpeed(motor, 2, 0);
        }
        break;
    }
}

void DRV8833_PWM_SetSpeed(MOTOR motor, int pin, int speed)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    ASSERT(pin == 1 || pin == 2);
    ASSERT(0 <= speed && speed <= 255);

    int channel = 0;

    if (motor == MOTOR_1)
    {
        channel = (pin == 1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;
    }
    if (motor == MOTOR_2)
    {
        channel = (pin == 1) ? LEDC_CHANNEL_2 : LEDC_CHANNEL_3;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void DRV8833_SetSpeed(MOTOR motor, int speed)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    ASSERT(0 <= speed && speed <= 255);

    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    target->speed = speed;

    DRV8833_PWM_SetSpeed(motor, target->current_direction, speed);
}