#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define NUM_SAMPLES     8000
#define TIMER_INTERVAL_US 125 // 8000Hz
#define PIN_MIC         ADC_CHANNEL_6 // (파트너님 핀 번호에 맞게 수정하세요)
#define PIN_BTN         GPIO_NUM_4    // (파트너님 핀 번호에 맞게 수정하세요)

// 💡 [핵심] 통신 핀 및 설정 (TM4C와 연결된 핀)
#define UART_PORT       UART_NUM_1
#define TX_PIN          17
#define RX_PIN          16
#define BAUD_RATE       230400 // TM4C 중계기 코드의 속도와 동일하게 맞춤

static const char *TAG = "DAQ_SYSTEM";

uint16_t adc_buffer[NUM_SAMPLES];
volatile int sample_index = 0;
volatile bool is_recording = false;

adc_oneshot_unit_handle_t adc1_handle;
SemaphoreHandle_t recording_done_sem = NULL;

// 고해상도 타이머 콜백 (ISR)
static void IRAM_ATTR timer_callback(void *arg) {
    if (is_recording && sample_index < NUM_SAMPLES) {
        int raw_val;
        adc_oneshot_read(adc1_handle, PIN_MIC, &raw_val);
        adc_buffer[sample_index++] = (uint16_t)raw_val;
    } else if (is_recording) {
        is_recording = false;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        // 💡 [핵심] xSemaphoreGiveFromISR: ISR(인터럽트 서비스 루틴)에서 세마포어를 해제합니다.
        // 사용 이유: 타이머 콜백(ISR)에서 데이터 수집이 완료되었음을 `app_main` 태스크에 알리기 위해 사용됩니다.
        //           ISR 컨텍스트에서 세마포어를 안전하게 '줄' 수 있는 유일한 방법입니다.
        // 장점:
        //    - ISR과 태스크 간의 안전한 동기화를 보장합니다.
        //    - ISR 내에서 태스크 전환을 유발할 수 있는 FreeRTOS API를 사용할 수 있도록 합니다.
        //      (여기서는 `xHigherPriorityTaskWoken`을 통해 더 높은 우선순위의 태스크가 깨어날 수 있음을 FreeRTOS 스케줄러에 알립니다.)
        xSemaphoreGiveFromISR(recording_done_sem, &xHigherPriorityTaskWoken);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "System Booting...");

    // 1. 세마포어 생성
    // 💡 [핵심] xSemaphoreCreateBinary: 이진 세마포어를 생성합니다.
    // 사용 이유: ISR(타이머 콜백)과 일반 태스크(app_main) 간의 동기화를 위해 사용됩니다.
    //           데이터 수집이 완료되면 ISR에서 세마포어를 '주고', app_main 태스크는 이 세마포어를 '받을' 때까지 대기하여
    //           데이터 수집 완료 시점을 정확히 알 수 있습니다.
    // 장점:
    //    - ISR과 태스크 간의 안전한 통신을 보장하여 경쟁 상태(Race Condition)를 방지합니다.
    //    - 태스크가 불필요하게 CPU 시간을 낭비하며 폴링(polling)하는 대신,
    //      이벤트(데이터 수집 완료)가 발생할 때까지 효율적으로 대기할 수 있도록 합니다.
    //    - 코드의 가독성과 유지보수성을 높여줍니다.
    recording_done_sem = xSemaphoreCreateBinary();

    // 2. ADC 초기화 (기존 코드와 동일)
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_11 };
    adc_oneshot_config_channel(adc1_handle, PIN_MIC, &config);

    // 3. 버튼 초기화
    gpio_set_direction(PIN_BTN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN, GPIO_PULLUP_ONLY);

    // 4. 💡 [핵심] 데이터 전송용 UART 1번 초기화
    uart_config_t uart_cfg = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT, &uart_cfg);
    uart_set_pin(UART_PORT, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 송신 버퍼를 넉넉하게 잡습니다.
    uart_driver_install(UART_PORT, 2048, NUM_SAMPLES * 2 + 100, 0, NULL, 0);

    // 5. 타이머 초기화
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "adc_timer"
    };
    esp_timer_create(&timer_args, &timer);

    ESP_LOGI(TAG, "DAQ Ready. Press Button to Start.");

    while (1) {
        if (gpio_get_level(PIN_BTN) == 0) {
            ESP_LOGI(TAG, "Recording Started...");
            sample_index = 0;
            is_recording = true;
            esp_timer_start_periodic(timer, TIMER_INTERVAL_US);
            
            // 데이터 수집 완료까지 대기
            // 💡 [핵심] xSemaphoreTake: 세마포어를 획득합니다.
            // 사용 이유: `app_main` 태스크가 `timer_callback` ISR에서 데이터 수집이 완료되었다는 신호를 보낼 때까지
            //           무한정(portMAX_DELAY) 대기합니다. 이를 통해 동기적인 작업 흐름을 구성할 수 있습니다.
            // 장점:
            //    - ISR이 작업을 완료할 때까지 태스크가 효율적으로 대기하도록 하여 CPU 낭비를 줄입니다.
            //    - 데이터 수집과 전송이라는 두 단계의 작업을 명확하게 분리하고 동기화합니다.
            //    - 특정 이벤트(여기서는 데이터 수집 완료)에 기반한 태스크 실행 순서를 보장합니다.
            xSemaphoreTake(recording_done_sem, portMAX_DELAY);
            esp_timer_stop(timer);

            ESP_LOGI(TAG, "Recording Done. Transmitting via UART 1...");

            // 💡 [핵심] 순수 바이너리 통신 구간
            // 1) 헤더 전송 (0xAA, 0x55)
            uint8_t header[2] = {0xAA, 0x55};
            uart_write_bytes(UART_PORT, header, 2);
            
            // 2) 데이터 버퍼 통째로 전송 (8000개 * 2바이트 = 16000바이트)
            uart_write_bytes(UART_PORT, (const char*)adc_buffer, sizeof(adc_buffer));

            ESP_LOGI(TAG, "Transmission Complete!");
            // 💡 [핵심] vTaskDelay: 현재 태스크를 지정된 시간(틱) 동안 지연시킵니다.
            // 사용 이유: 버튼이 한 번 눌렸을 때 여러 번 인식되는 것을 방지하기 위한 디바운싱(debouncing) 목적으로 사용됩니다.
            //           또한, 다음 녹음 시작까지 일정한 대기 시간을 두어 시스템의 안정성을 확보합니다.
            // 장점:
            //    - CPU 시간을 효율적으로 사용합니다. 태스크가 활성 대기(busy-waiting)하는 대신,
            //      지연 시간 동안 FreeRTOS 스케줄러가 다른 태스크를 실행할 수 있도록 양보합니다.
            //    - 하드웨어 제어(버튼 입력 처리)에서 안정성을 높이고 원치 않는 중복 동작을 방지합니다.
            vTaskDelay(pdMS_TO_TICKS(1000)); // 버튼 디바운싱 및 대기
        }
        // 💡 [핵심] vTaskDelay: 현재 태스크를 지정된 시간(틱) 동안 지연시킵니다.
        // 사용 이유: `while(1)` 루프가 무한정 빠르게 실행되면서 CPU 자원을 독점하는 것을 방지합니다.
        //           짧은 지연을 통해 다른 태스크나 백그라운드 작업이 실행될 수 있도록 CPU 시간을 양보합니다.
        // 장점:
        //    - 시스템의 전체적인 반응성과 안정성을 향상시킵니다.
        //    - 불필요한 CPU 사이클 낭비를 줄여 전력 효율을 높이고 발열을 감소시킵니다.
        //    - FreeRTOS 스케줄러가 효율적으로 태스크를 관리하고 전환할 수 있도록 합니다.
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}