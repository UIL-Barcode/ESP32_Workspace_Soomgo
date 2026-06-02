#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
    
// [1] 하드웨어 핀 및 설정 매크로 (타겟 칩셋에 맞게 핀 번호 수정 필요)
#define SPI_HOST_ID     SPI2_HOST // pin이 아님, SPI2_HOST는 ESP32의 SPI2 버스를 의미 (VSPI 또는 HSPI로도 불림)
#define PIN_NUM_MISO    GPIO_NUM_12
#define PIN_NUM_MOSI    GPIO_NUM_13
#define PIN_NUM_CLK     GPIO_NUM_14
#define PIN_NUM_CS      GPIO_NUM_15

#define SPI_CLOCK_HZ    (1 * 1000 * 1000) // 1 MHz
#define TX_PAYLOAD_SIZE 2                 // 2 Bytes

static const char *TAG = "SPI_MASTER";

void app_main(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;

    // [2] SPI 버스 설정 (Master 측 물리적 핀 연결)
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32 // DMA 미사용 시 최소 요구 버퍼 크기
    };

    // [3] SPI 디바이스 설정 (통신할 Slave의 프로토콜 규격)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,                      // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = PIN_NUM_CS,     // 전송 시 CS 핀 자동 제어 (Active Low)
        .queue_size = 1,                // 폴링 전송이므로 큐 크기는 1로 충분
    };

    // [4] SPI 버스 초기화 및 디바이스 추가 (DMA 비활성화 모드)
    ret = spi_bus_initialize(SPI_HOST_ID, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);
    
    ret = spi_bus_add_device(SPI_HOST_ID, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "SPI Master initialized successfully.");

    // [5] 트랜잭션 변수 및 페이로드 초기화
    spi_transaction_t t;
    uint8_t tx_data[TX_PAYLOAD_SIZE] = {0};
    uint8_t counter = 0;

    while (1) {
        // 송신 데이터 버퍼 갱신 (예: [0x00, 0x01] -> [0x02, 0x03])
        tx_data[0] = counter++;
        tx_data[1] = counter++;

        // 트랜잭션 구조체 메모리 초기화 및 세팅
        memset(&t, 0, sizeof(t));
        t.length = TX_PAYLOAD_SIZE * 8; // length 필드는 Byte가 아닌 Bit 단위로 입력해야 함
        t.tx_buffer = tx_data;
        t.rx_buffer = NULL;             // 수신 데이터가 필요 없으므로 NULL 처리

        // [6] 폴링 방식으로 데이터 전송 실행
        ret = spi_device_polling_transmit(spi, &t);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Transmitted: [0x%02X, 0x%02X]", tx_data[0], tx_data[1]);
        } else {
            ESP_LOGE(TAG, "SPI Transmission failed");
        }

        // 1초(1000ms) 대기 (FreeRTOS 스케줄러 블로킹 해제)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}