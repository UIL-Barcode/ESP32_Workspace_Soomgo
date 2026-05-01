#include "ir_rmt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_rx.h"

// ISR(인터럽트) 콜백 함수: RMT 하드웨어가 신호를 다 받으면 큐로 알려줍니다.
static bool IRAM_ATTR ir_rx_done_cb(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    
    // 수신된 심볼(펄스)의 개수를 큐에 담아 메인 태스크로 보냅니다.
    xQueueSendFromISR(queue, &edata->num_symbols, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

// IR 수신기 초기화
rmt_channel_handle_t init_ir_rx(const ir_rx_config_t *config) {
    rmt_channel_handle_t rx_chan = NULL;
    
    // RMT 채널 설정 (1MHz 해상도 = 1us 측정 단위)
    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = config->resolution_hz,
        .mem_block_symbols = 64, // 리모컨 신호 하나를 담기에 넉넉한 64 묶음
        .gpio_num = config->gpio_pin,
    };
    if (rmt_new_rx_channel(&rx_chan_config, &rx_chan) != ESP_OK) return NULL;

    // 콜백 함수 등록 (비동기 수신용)
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = ir_rx_done_cb,
    };
    rmt_rx_register_event_callbacks(rx_chan, &cbs, config->rx_queue);

    // 하드웨어 시작
    rmt_enable(rx_chan);
    return rx_chan;
}
