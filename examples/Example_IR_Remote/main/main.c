#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_rx.h"
#include "esp_err.h"

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

// ============================================================================
// [2] 메인 어플리케이션 영역
// ============================================================================

void app_main(void) {
    // 1. 수신 대기를 위한 큐 생성
    QueueHandle_t ir_queue = xQueueCreate(1, sizeof(size_t));

    // 2. 모듈 설정 주입
    ir_rx_config_t my_ir = {
        .gpio_pin = 18,             // IR 리시버 OUT 핀
        .resolution_hz = 1000000,   // 1MHz (1us 단위)
        .rx_queue = ir_queue
    };

    rmt_channel_handle_t rx_channel = init_ir_rx(&my_ir);
    if (!rx_channel) {
        printf("IR Receiver Init Failed!\n");
        return;
    }
    printf("ESP-IDF RMT IR Receiver Started on GPIO %d\n", my_ir.gpio_pin);

    // 수신된 데이터를 담을 버퍼 배열 (ESP-IDF의 특수 구조체)
    rmt_symbol_word_t raw_symbols[64];

    while (1) {
        rmt_receive_config_t receive_config = {
            .signal_range_min_ns = 0,            // 노이즈 필터 미사용 (IR 수신 모듈이 자체적으로 필터링함)
            .signal_range_max_ns = 12000000,     // 12ms (12,000,000 ns) 동안 신호가 안 바뀌면 '수신 끝'으로 간주
        };

        // 3. 하드웨어에 "수신 시작해!" 라고 명령 (명령 후 바로 다음 줄로 넘어감 = Non-blocking)
        rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config);

        // 4. ISR 콜백에서 큐를 던져줄 때까지 대기
        size_t num_symbols = 0;
        if (xQueueReceive(my_ir.rx_queue, &num_symbols, portMAX_DELAY) == pdTRUE) {
            
            // NEC 프로토콜은 보통 1개의 헤더 + 32개의 비트 + 1개의 종료 비트로 구성 (약 34개 묶음)
            if (num_symbols >= 33) {
                
                // [헤더 확인] 대략 9000us, 4500us 근처인지 오차를 감안해서 확인합니다.
                if (raw_symbols[0].duration0 > 8000 && raw_symbols[0].duration1 > 4000) {
                    
                    uint32_t decoded_data = 0;
                    
                    // 인덱스 1부터 32까지의 비트 데이터를 순회하며 해독합니다.
                    for (int i = 1; i <= 32; i++) {
                        // 신호 사이의 공백(duration1) 길이가 1000us 이상이면 '1', 아니면 '0'으로 판별합니다.
                        if (raw_symbols[i].duration1 > 1000) {
                            // 비트가 '1'이면 해당 자리수에 1을 채워 넣습니다. (NEC는 LSB부터 전송)
                            decoded_data |= (1UL << (i - 1)); 
                        }
                    }
                    
                    // ⭐️ [최종 출력] 32비트 데이터를 16진수로 출력합니다.
                    printf("\n✅ 버튼 수신 완료! HEX: 0x%08lX\n", decoded_data);
                    
                    // 참고: NEC 프로토콜은 [주소(8) + 주소반전(8) + 명령(8) + 명령반전(8)] 구조입니다.
                    uint8_t command = ((decoded_data >> 16) & 0xFF);
                    printf("   명령어(Command) 값: 0x%02X\n", command);

                    for (int i = 0; i < sizeof(ir_codes) / sizeof(ir_codes[0]); i++)
                    {
                        if (ir_codes[i].hex == ((decoded_data >> 16) & 0xFFFF))
                        {
                            printf("   리모컨 값 : %c\n", ir_codes[i].code);
                        }                    
                    }
                }                
                // 버튼을 계속 꾹 누르고 있을 때 발생하는 반복 신호(Repeat Code)
                else if (raw_symbols[0].duration0 > 8000 && raw_symbols[0].duration1 > 2000) {
                    printf("🔁 버튼 계속 누르는 중...\n");
                } 
                else {
                    printf("⚠️ 알 수 없는 프로토콜 형식이거나 노이즈입니다.\n");
                }
            }
        }
    }
}