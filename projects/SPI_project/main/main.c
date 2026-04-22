#include <stdio.h> // For printf
#include <string.h> // For memset
#include "driver/spi_master.h" // For SPI communication
#include "driver/gpio.h" // For GPIO pin definitions
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#define MY_MIN(x, y) ((x) < (y) ? (x) : (y))
#define MY_MAX(x, y) ((x) > (y) ? (x) : (y))

#define PIN_NUM_MISO    GPIO_NUM_12
#define PIN_NUM_MOSI    GPIO_NUM_13
#define PIN_NUM_CLK     GPIO_NUM_14
#define PIN_NUM_CS      GPIO_NUM_15

// 핀 및 채널 정의
#define JOYSTICK_X_ADC_CHAN ADC_CHANNEL_6 // GPIO34
#define JOYSTICK_Y_ADC_CHAN ADC_CHANNEL_7 // GPIO35
#define JOYSTICK_SW_PIN     GPIO_NUM_32   // 버튼 핀

#define TX_PAYLOAD_SIZE 2                 // 2 Bytes

spi_device_handle_t spi;
adc_oneshot_unit_handle_t adc1_handle;

// [로직 추가] 점의 초기 좌표 설정 (0부터 시작하는 인덱스 기준)
int current_row = 3;  // 4번째 줄
int current_col = 16; // 17번째 칸

uint32_t map[8] = {
    0xF000000F, // Row 1 (0, 0)
    0x0F0000F0, // Row 2
    0x00F00F00, // Row 3
    0x000FF000, // Row 4
    0x00000000, // Row 5
    0x00000000, // Row 6
    0x00000000, // Row 7
    0x80000000  // Row 8
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

void SPI_Master_Init() {
    esp_err_t ret;

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
}


void ADC_Init() {
    // ADC 초기화 코드 (필요한 경우)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << JOYSTICK_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 2. ADC One-Shot 핸들 초기화
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 3. ADC 채널 세팅 (12비트 해상도, 11dB 감쇠기로 0~3.3V 전체 범위 측정)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_X_ADC_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_Y_ADC_CHAN, &config));

}


void app_main(void)
{
    int x_val = 0;
    int y_val = 0;
    int sw_val = 0;

    SPI_Master_Init();
    ADC_Init();



    while (1) {
        
       // 아날로그 값 읽기 (0 ~ 4095)
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOYSTICK_X_ADC_CHAN, &x_val));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOYSTICK_Y_ADC_CHAN, &y_val));
        
        // 디지털 스위치 상태 읽기 (눌리면 0, 안 눌리면 1)
        sw_val = gpio_get_level(JOYSTICK_SW_PIN); 

 //       ESP_LOGI(TAG, "X: %4d | Y: %4d | SW: %d", x_val, y_val, sw_val);

        // 50ms마다 측정 (20Hz 샘플링)
    //    vTaskDelay(pdMS_TO_TICKS(50));

//        current_col = (x_val * 31) / 4095; // 0~31 범위로 매핑
  //      current_row = (y_val * 7) / 4095;  // 0~7 범위로 매핑
        if(x_val<1000) current_col -= 1; // x_val이 1000보다 작으면 왼쪽으로 이동
        else if(x_val>3000) current_col += 1; // x_val이 3000보다 크면 오른쪽으로 이동
        if(y_val<1000) current_row -= 1; // y_val이 1000보다 작으면 아래로 이동
        else if(y_val>3000) current_row += 1; // y_val이 3000보다 크면 위로 이동
        
        current_col = MY_MIN(MY_MAX(current_col, 0), 31); // x_val이 0~31 범위를 벗어나지 않도록 보장
        current_row = MY_MIN(MY_MAX(current_row, 0), 7);  // y_val이 0~7 범위를 벗어나지 않도록 보장

        // 1. 기존 화면 지우기 (버퍼의 모든 잔상을 0으로 덮어씀)
        memset(map, 0, sizeof(map));

        // 2. 현재 좌표에 점 찍기
        // 1UL(Unsigned Long 타입의 숫자 1)을 current_col 만큼 시프트하여 해당 비트만 1로 켭니다.
        map[current_row] = (1UL << current_col);

        // 3. 디스플레이 화면 갱신
        print_map(spi);
        
        // 4. 좌표 이동 계산 (왼쪽으로 이동)
        // 비트 연산에서 숫자가 작아지는 방향을 왼쪽으로 가정합니다.
        // current_col--; 

        // // 5. 경계 도달 시 위치 전환 (해당 row의 가장 왼쪽을 넘어간 경우)
        // if (current_col < 0) {
        //     current_col = 31;  // 다음 row의 가장 오른쪽 끝(31번째 칸)으로 래핑
        //     current_row++;     // 아래 row로 이동
            
        //     // 만약 가장 아래쪽 줄(row 7)마저 넘어갔다면 맨 윗줄(row 0)로 복귀
        //     if (current_row > 7) {
        //         current_row = 0;
        //     }
        // }
        // current_col = (current_col + 1) % 32; // current_col이 0~31 범위를 벗어나지 않도록 보장
        // if (current_col == 0) {
        //     current_row = (current_row + 1) % 8; // current_row가 0~7 범위를 벗어나지 않도록 보장
        // }
        // 1초(1000ms) 대기 (FreeRTOS 스케줄러 블로킹 해제)
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
