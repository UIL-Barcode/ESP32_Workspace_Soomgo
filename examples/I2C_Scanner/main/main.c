#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_PORT     I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  100000

static const char *TAG = "I2C_SCANNER";

/* I2C Master 초기화 */
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK) return err;
    
    return i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
}

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C Master initialized successfully.");
    ESP_LOGI(TAG, "Starting I2C device scan...\n");

    int devices_found = 0;

    // 출력 포맷팅 (리눅스 i2cdetect 유틸리티 스타일)
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    printf("00:         ");

    // 7-bit 주소 공간 탐색 (1 ~ 127)
    for (uint8_t address = 1; address < 128; address++) {
        if (address % 16 == 0) {
            printf("\n%02x:", address);
        }

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        // 대상 주소로 Write 모드 진입 시도
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        // 명령어 전송 후 응답(ACK) 대기
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(10));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            // ACK 수신 (장치 존재함)
            printf(" %02x", address);
            devices_found++;
        } else if (ret == ESP_ERR_TIMEOUT) {
            // 타임아웃 오류
            printf(" UU");
        } else {
            // NACK 수신 (장치 없음)
            printf(" --");
        }
    }
    
    printf("\n\nScan completed. %d device(s) found.\n", devices_found);
}