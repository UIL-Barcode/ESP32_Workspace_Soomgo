#include "uil_DRV8833.h"

#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/ledc.h"

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

// 전역 모터 설정 인스턴스 (모터 1, 모터 2)
DRV8833_config_t DRV8833_M1;
DRV8833_config_t DRV8833_M2;

// 드라이버의 활성화 상태 및 STBY 핀 번호 저장
bool is_ready = false;
int stby_pin;

/**
 * @brief DRV8833 모터 드라이버를 활성화하고 대기 모드(Standby)를 해제합니다.
 */
void DRV8833_Enable(int pin)
{
    // 이미 같은 핀으로 활성화되어 있으면 무시
    if (is_ready && stby_pin == pin)
        return;
    
    stby_pin = pin;
    
    // STBY 핀 초기화 및 출력 모드 설정
    gpio_reset_pin(stby_pin);
    gpio_set_direction(stby_pin, GPIO_MODE_OUTPUT);
    
    // STBY 핀에 High(1)를 인가하여 드라이버 활성화
    gpio_set_level(stby_pin, 1);

    is_ready = true;
}

/**
 * @brief DRV8833 모터 드라이버를 비활성화하여 대기 모드(Standby)로 진입합니다.
 */
void DRV8833_Disable()
{
    if (!is_ready)
        return;
        
    // STBY 핀에 Low(0)를 인가하여 드라이버 비활성화(대기 모드)
    gpio_set_level(stby_pin, 0);
    is_ready = false;
}

/**
 * @brief 두 개의 핀 번호로 초기화된 기본 모터 설정 구조체를 반환합니다.
 */
DRV8833_config_t DRV8833_GetDefault(int pin1, int pin2)
{
    DRV8833_config_t default_config = {
        ROTATION_CW,       // 기본 물리적 방향: 시계 방향(정방향)
        DIRECTION_FORWARD, // 기본 구동 방향: 전진
        pin1,              // 입력 핀 1 할당
        pin2,              // 입력 핀 2 할당
        0                  // 초기 속도: 정지(0)
    };
    return default_config;
}

/**
 * @brief 전달된 설정 구조체의 데이터를 복사하여 특정 모터(M1/M2)의 설정을 초기화합니다.
 */
void DRV8833_InitMotor(MOTOR motor, DRV8833_config_t* config)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    // 선택된 모터에 대응하는 전역 구조체 포인터 획득
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    // 인자로 받은 설정값을 전역 구조체에 복사
    (void)memcpy(target, config, sizeof(DRV8833_config_t));
}

/**
 * @brief 두 개의 모터를 각각의 설정 구조체로 한 번에 초기화합니다.
 */
void DRV8833_Init(DRV8833_config_t* config1, DRV8833_config_t* config2)
{
    DRV8833_InitMotor(MOTOR_1, config1);
    DRV8833_InitMotor(MOTOR_2, config2);
}

/**
 * @brief 모터를 기본값으로 초기화하고, ESP32의 하드웨어 타이머(LEDC)를 이용해 PWM 핀 설정을 완료합니다.
 */
void DRV8833_Init_Default(MOTOR motor, int pin1, int pin2)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);

    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;
    DRV8833_config_t default_config = DRV8833_GetDefault(pin1, pin2);
    // 설정값 적용
    (void)memcpy(target, &default_config, sizeof(DRV8833_config_t));
    
    // 1. 모터 속도 제어용 PWM(LEDC) 타이머 공통 설정
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,     // 타이머 0 사용
        .duty_resolution  = LEDC_TIMER_8_BIT, // 8비트 해상도 (속도 제어 범위: 0 ~ 255)
        .freq_hz          = 5000,             // PWM 주파수 5kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. 모터 입력 핀 1에 대한 PWM 채널 설정 및 연결
    ledc_channel_config_t ledc_channel_0 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (motor == MOTOR_1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = target->in_pin1, // 핀 1 연결
        .duty           = 0,               // 초기 듀티 사이클 0 (정지)
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_0);

    // 3. 모터 입력 핀 2에 대한 PWM 채널 설정 및 연결
    ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = (motor == MOTOR_1) ? LEDC_CHANNEL_1 : LEDC_CHANNEL_3,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = target->in_pin2, // 핀 2 연결
        .duty           = 0,               // 초기 듀티 사이클 0 (정지)
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_1);
}

/**
 * @brief 모터의 물리적 장착 방향(CW 또는 CCW)을 설정합니다.
 */
void DRV8833_SetRotation(MOTOR motor, DEFAULT_ROTATION rotation)
{
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    target->rotation = rotation; // 설정 구조체에 물리적 방향 정보 저장
}

/**
 * @brief 모터의 진행 방향(전진/후진)을 결정하고, 장착 방향에 맞춰 각 핀에 속도(PWM)를 올바르게 인가합니다.
 */
void DRV8833_SetDirection(MOTOR motor, DIRECTION direction)
{
    ASSERT(is_ready); // 드라이버가 활성화되어 있어야 함
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;
    
    // pin_p : 모터의 정방향(+) 회전을 담당하는 핀 인덱스
    // pin_n : 모터의 역방향(-) 회전을 담당하는 핀 인덱스
    int pin_p = 0, pin_n = 0;

    target->direction = direction; // 현재 논리적 방향 업데이트

    // 물리적 모터 장착 방식이 정방향(CW)이면 1번 핀이 (+), 2번 핀이 (-) 역할
    // 역방향(CCW)으로 장착되었으면 2번 핀이 (+), 1번 핀이 (-) 역할로 스위칭
    pin_p = (target->rotation == ROTATION_CW) ? 1 : 2;
    pin_n = (target->rotation == ROTATION_CW) ? 2 : 1;

    switch(direction)
    {
        case DIRECTION_FORWARD:
        {
            // 전진: (-) 핀의 출력을 끄고, (+) 핀에 현재 설정된 속도를 인가
            DRV8833_PWM_SetSpeed(motor, pin_n, 0);
            DRV8833_PWM_SetSpeed(motor, pin_p, target->speed);
        }
        break;
        case DIRECTION_BACKWARD:
        {
            // 후진: (+) 핀의 출력을 끄고, (-) 핀에 현재 설정된 속도를 인가
            DRV8833_PWM_SetSpeed(motor, pin_p, 0);
            DRV8833_PWM_SetSpeed(motor, pin_n, target->speed);
        }
        break;
        default:
        {
            // 예외 및 정지: 양쪽 핀 출력을 모두 0으로 만들어 정지(브레이크)
            DRV8833_PWM_SetSpeed(motor, 1, 0);
            DRV8833_PWM_SetSpeed(motor, 2, 0);
        }
        break;
    }
}

/**
 * @brief 하드웨어 추상화 계층으로, 선택한 모터의 특정 핀(1 또는 2)에 실제 PWM 듀티 사이클을 기록합니다.
 */
void DRV8833_PWM_SetSpeed(MOTOR motor, int pin, int speed)
{
    ASSERT(is_ready); // 드라이버가 활성화되어 있어야 함
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    ASSERT(pin == 1 || pin == 2);
    ASSERT(0 <= speed && speed <= 255); // 속도는 8비트 해상도 내 존재해야 함

    int channel = 0;

    // 모터 번호와 핀 번호의 조합에 따라 ESP32 LEDC에 매핑된 채널을 결정
    if (motor == MOTOR_1)
    {
        channel = (pin == 1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;
    }
    if (motor == MOTOR_2)
    {
        channel = (pin == 1) ? LEDC_CHANNEL_2 : LEDC_CHANNEL_3;
    }

    // 결정된 하드웨어 채널에 듀티 사이클(속도) 설정 및 반영
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

/**
 * @brief 모터의 전체 속도를 갱신하고, 현재 방향 정보를 바탕으로 해당 핀에 즉시 속도를 인가합니다.
 */
void DRV8833_SetSpeed(MOTOR motor, int speed)
{
    ASSERT(is_ready); // 드라이버가 활성화되어 있어야 함
    ASSERT(motor == MOTOR_1 || motor == MOTOR_2);
    ASSERT(0 <= speed && speed <= 255);

    DRV8833_config_t* target = (motor == MOTOR_1) ? &DRV8833_M1 : &DRV8833_M2;

    target->speed = speed; // 현재 속도 변수 갱신

    // 현재 설정된 논리 방향(DIRECTION_FORWARD/BACKWARD)에 맞춰 속도(PWM)를 갱신합니다.
    DRV8833_PWM_SetSpeed(motor, target->direction, speed);
}