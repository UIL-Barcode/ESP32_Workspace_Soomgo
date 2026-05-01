#ifndef IR_RMT_H
#define IR_RMT_H

#include <stdint.h>
#include "freertos/queue.h"
#include "driver/rmt_rx.h"

// ============================================================================
// [1] 드라이버 영역 (나중에 ir_receiver_c 폴더로 옮길 부분)
// ============================================================================
typedef struct {
    uint16_t hex;
    char code;
} ir_code_t;

const ir_code_t ir_codes[] =
{
    {0xBA45, '1'},
    {0xB946, '2'},
    {0xB847, '3'},
    {0xBB44, '4'},
    {0xBF40, '5'},
    {0xBC43, '6'},
    {0xF807, '7'},
    {0xEA15, '8'},
    {0xF609, '9'},
    {0xE916, '*'},
    {0xE619, '0'},
    {0xF20D, '#'},
    {0xE718, 'U'},
    {0xF708, 'L'},
    {0xE31C, 'O'},
    {0xA55A, 'R'},
    {0xAD52, 'D'}
};

typedef struct {
    int gpio_pin;
    uint32_t resolution_hz;
    QueueHandle_t rx_queue; // 수신 완료 알림을 받을 FreeRTOS 큐
} ir_rx_config_t;

rmt_channel_handle_t init_ir_rx(const ir_rx_config_t *config);

#endif // IR_RMT_H