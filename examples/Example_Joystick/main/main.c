#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "JOYSTICK";

// 핀 및 채널 정의
#define JOYSTICK_X_ADC_CHAN ADC_CHANNEL_6 // GPIO34
#define JOYSTICK_Y_ADC_CHAN ADC_CHANNEL_7 // GPIO35
#define JOYSTICK_SW_PIN     GPIO_NUM_32   // 버튼 핀

void app_main(void)
{ 
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << JOYSTICK_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 2. ADC One-Shot 핸들 초기화
    adc_oneshot_unit_handle_t adc1_handle;
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

    int x_val = 0;
    int y_val = 0;
    int sw_val = 0;

    // 4. One-Shot 측정 루프
    while (1) {
        // 아날로그 값 읽기 (0 ~ 4095)
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOYSTICK_X_ADC_CHAN, &x_val));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOYSTICK_Y_ADC_CHAN, &y_val));
        
        // 디지털 스위치 상태 읽기 (눌리면 0, 안 눌리면 1)
        sw_val = gpio_get_level(JOYSTICK_SW_PIN); 

        ESP_LOGI(TAG, "X: %4d | Y: %4d | SW: %d", x_val, y_val, sw_val);

        // 50ms마다 측정 (20Hz 샘플링)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}