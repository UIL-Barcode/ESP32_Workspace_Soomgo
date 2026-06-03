#include "uil_L298N.h"
#include <string.h>
#include <assert.h>

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

L298N_Config L298N_M1;
L298N_Config L298N_M2;

L298N_Config L298N_GetDefault(int en, int pin1, int pin2)
{
    L298N_Config default_config = {
        ROTATION_CW,
        DIRECTION_FORWARD,
        en,
        pin1,
        pin2,
        0
    };
    return default_config;
}

void L298N_InitMotor(MOTOR motor, L298N_Config* config)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    L298N_Config* target = (motor == MOTOR_1) ? &L298N_M1 : &L298N_M2;

    (void)memcpy(target, config, sizeof(L298N_Config));
}

void L298N_Init(L298N_Config* config1, L298N_Config* config2)
{
    L298N_InitMotor(MOTOR_1, config1);
    L298N_InitMotor(MOTOR_2, config2);
}

void L298N_Init_Default(MOTOR motor, int en, int pin1, int pin2)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);

    L298N_Config* target = (motor == MOTOR_1) ? &L298N_M1 : &L298N_M2;
    L298N_Config default_config = L298N_GetDefault(en, pin1, pin2);
    (void)memcpy(target, &default_config, sizeof(L298N_Config));
    
    gpio_reset_pin(target->in_pin1);
    gpio_reset_pin(target->in_pin2);
    gpio_set_direction(target->in_pin1, GPIO_MODE_OUTPUT);
    gpio_set_direction(target->in_pin2, GPIO_MODE_OUTPUT);
    
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
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (motor == MOTOR_1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = target->en_pin,
        .duty           = 0, // 초기 속도 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void L298N_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    L298N_Config* target = (motor == MOTOR_1) ? &L298N_M1 : &L298N_M2;

    target->rotation = rotation;
}

void L298N_SetDirection(MOTOR motor, DIRECTION direction)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    L298N_Config* target = (motor == MOTOR_1) ? &L298N_M1 : &L298N_M2;
    int value1 = 0, value2 = 0;

    switch(direction)
    {
        case DIRECTION_FORWARD:
        {
            value1 = (target->rotation == ROTATION_CW) ? 1 : 0;
            value2 = (target->rotation == ROTATION_CW) ? 0 : 1;
        }
        break;
        case DIRECTION_BACKWARD:
        {
            value1 = (target->rotation == ROTATION_CW) ? 0 : 1;
            value2 = (target->rotation == ROTATION_CW) ? 1 : 0;
        }
        break;
        default:
        {
            value1 = 0;
            value2 = 0;
        }
        break;
    }

    gpio_set_level(target->in_pin1, value1);
    gpio_set_level(target->in_pin2, value2);
}

void L298N_SetSpeed(MOTOR motor, int speed)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    ASSERT(0 <= speed && speed <= 255);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}