#include <stdio.h> // For printf
#include <string.h> // For memset
#include "driver/spi_master.h" // For SPI communication
#include "driver/gpio.h" // For GPIO pin definitions


#define PIN_NUM_MISO    GPIO_NUM_12
#define PIN_NUM_MOSI    GPIO_NUM_13
#define PIN_NUM_CLK     GPIO_NUM_14
#define PIN_NUM_CS      GPIO_NUM_15

#define TX_PAYLOAD_SIZE 2                 // 2 Bytes

uint32_t map[8] = {
    0xF0000000, // Row 1
    0x0F000000, // Row 2
    0x00F00000, // Row 3
    0x000F0000, // Row 4
    0x0000F000, // Row 5
    0x00000F00, // Row 6
    0x000000F0, // Row 7
    0x0000000F  // Row 8
};

// 8byte SPI 전송 함수
void transmit_spi_8byte(spi_device_handle_t spi, uint8_t data[8]) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // Clear the transaction structure
    t.length = 8 * 8; // Length in bits
    t.tx_buffer = data; // Data to transmit
    t.rx_buffer = NULL; // No need for a receive buffer

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    if (ret != ESP_OK) {
        printf("SPI Transmission failed: %s\n", esp_err_to_name(ret));
    } else {
        printf("SPI Transmission successful: [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
}

// 2byte SPI 전송 함수
void transmit_spi_2byte(spi_device_handle_t spi, uint8_t data_high, uint8_t data_low) {
    uint8_t data[8] = {0};
    for (int i = 0; i < 4; i++)
    {
        data[2*i] = data_high; // 명령어
        data[2*i + 1] = data_low; // 데이터
    }
    transmit_spi_8byte(spi, data);
}


void print_map(spi_device_handle_t spi)
{
    for (int i = 0; i < 8; i++) {
        uint8_t data[8] = {0};
        data[0] = i + 1; // Row address
        data[2] = i + 1; // Row address
        data[4] = i + 1; // Row address
        data[6] = i + 1; // Row address
        for (int j = 0; j < 4; j++) {
            data[2*j + 1] = (map[i] >> (24 - 8*j)) & 0xFF; // Extract byte from map
        }
        transmit_spi_8byte(spi, data);
    }
}

void app_main(void)
{
    esp_err_t ret;

    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz = 26 * 1000 * 1000,     //Clock out at 26 MHz
#else
                .clock_speed_hz = 10 * 1000 * 1000,     //Clock out at 10 MHz
#endif
        .mode = 0,                              //SPI mode 0
        .spics_io_num = PIN_NUM_CS,             //CS pin
        .queue_size = 1                        //We want to be able to queue 7 transactions at a time
//        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };

        //Initialize the SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi); //통신 프로토콜대로 처리하라는 명령
    ESP_ERROR_CHECK(ret);

//   sendCommandAll(0x0C, 0x01);  // Exit shutdown mode
//   sendCommandAll(0x09, 0x00);  // Disable decode mode
//   sendCommandAll(0x0A, 0x0F);  // Set intensity (0x00 to 0x0F)
//   sendCommandAll(0x0B, 0x07);  // Set scan limit (0 to 7)
//   sendCommandAll(0x0F, 0x00);  // Disable display test mode
    // [5] 트랜잭션 변수 및 페이로드 초기화
    spi_transaction_t t;
    uint8_t tx_data[TX_PAYLOAD_SIZE] = {0};
//    uint8_t counter = 0;

    //  char cmd = 0x0C;
    // 초기화 명령어 전송
    transmit_spi_2byte(spi, 0x0C, 0x01);
    transmit_spi_2byte(spi, 0x09, 0x00);
    transmit_spi_2byte(spi, 0x0A, 0x0F);
    transmit_spi_2byte(spi, 0x0B, 0x07);
    transmit_spi_2byte(spi, 0x0F, 0x00);

    uint8_t data[8] = {0x00}; //최초의 한 바이트만 0으로 초기화하면 전체 배열이 0으로 초기화됨


    while (1) {
        for(uint8_t i = 0; i < 8; i++) {
            data[0] = i+1; //행 주소 (0)
            data[2] = i+1;      
            data[4] = i+1;
            data[6] = i+1;
            transmit_spi_8byte(spi, data); // 배열의 이름은 해당 배열의 주소를 의미
        }

        print_map(spi);
            
        // 1초(1000ms) 대기 (FreeRTOS 스케줄러 블로킹 해제)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
