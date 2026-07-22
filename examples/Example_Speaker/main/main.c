#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_dsp.h"
#include "dsps_fft2r_platform.h"
#include "dsps_fft2r.h"
#include "driver/i2s_std.h"

#define SAMPLE_RATE         8000
// 💡 [수정] FFT 제약 조건(샘플 수는 2의 거듭제곱이어야 함)을 만족시키기 위해 NUM_SAMPLES를 32768로 고정합니다.
// 이렇게 하면 최대 녹음 시간은 약 4.1초(32768 / 8000)가 됩니다.
#define NUM_SAMPLES         32768 // 2^15
#define MAX_RECORDING_MSEC  ((NUM_SAMPLES * 1000) / SAMPLE_RATE)

#define TIMER_INTERVAL_US   (1000000 / SAMPLE_RATE)

#define PIN_MIC         ADC_CHANNEL_6
#define PIN_BTN         GPIO_NUM_4

// 💡 [핵심] 스피커 출력 핀 설정
#define I2S_PORT        I2S_NUM_0
#define I2S_PIN_BCK     -1 // -1로 설정 시 자동 할당
#define I2S_PIN_WS      -1 // -1로 설정 시 자동 할당
#define I2S_PIN_DOUT    26 // Left 채널 (스피커 핀에 맞게 수정)
#define I2S_PIN_MCK     -1 // 사용하지 않음

// 💡 [핵심] 노이즈 필터 선택
#define APPLY_MOVING_AVERAGE_FILTER false // 간단한 이동 평균 필터 (Hiss 노이즈 감소용)
#define APPLY_FFT_NOTCH_FILTER      true  // FFT 기반 노치 필터 ('삐~' 소리 같은 특정 주파수 노이즈 제거용)

#if APPLY_FFT_NOTCH_FILTER
// 💡 제거할 노이즈의 중심 주파수 (Hz)와 대역폭 (Hz) 설정
// 예: 4000Hz 주변의 노이즈를 제거
#define NOTCH_FILTER_FREQ   4000
#define NOTCH_FILTER_WIDTH  200
#endif

static const char *TAG = "MIC_TO_SPEAKER";

int16_t audio_buffer[NUM_SAMPLES]; // ADC와 I2S가 공유하는 버퍼
volatile int sample_index = 0;
volatile bool is_recording = false;

adc_oneshot_unit_handle_t adc1_handle;
i2s_chan_handle_t tx_handle;
SemaphoreHandle_t recording_done_sem = NULL;

// 고해상도 타이머 콜백 (ISR)
static void IRAM_ATTR timer_callback(void *arg) {
    if (is_recording && sample_index < NUM_SAMPLES) {
        int raw_val;
        adc_oneshot_read(adc1_handle, PIN_MIC, &raw_val);
        audio_buffer[sample_index++] = (uint16_t)raw_val;
    } else if (is_recording) {
        is_recording = false;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(recording_done_sem, &xHigherPriorityTaskWoken);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "System Booting...");

    // 1. 세마포어 생성
    recording_done_sem = xSemaphoreCreateBinary();

    // 2. ADC 초기화
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_12 };
    adc_oneshot_config_channel(adc1_handle, PIN_MIC, &config);

    // 3. 버튼 초기화
    gpio_set_direction(PIN_BTN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN, GPIO_PULLUP_ONLY);

    // 4. 💡 [핵심] 스피커 출력을 위한 I2S 초기화
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_PIN_MCK,
            .bclk = I2S_PIN_BCK,
            .ws = I2S_PIN_WS,
            .dout = I2S_PIN_DOUT,
            .din = -1, // 입력은 사용하지 않음
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false, },
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);

    // 5. 타이머 초기화
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "adc_timer"
    };
    esp_timer_create(&timer_args, &timer);

    ESP_LOGI(TAG, "System Ready. Press and hold the button to record for up to %.1f seconds.", (float)MAX_RECORDING_MSEC / 1000.0);

    while (1) {
        // 버튼이 눌릴 때까지 대기
        while (gpio_get_level(PIN_BTN) != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // 버튼이 눌리면 녹음 시작
        if (is_recording == false) {
            ESP_LOGI(TAG, "Recording Started...");
            sample_index = 0;
            is_recording = true;
            esp_timer_start_periodic(timer, TIMER_INTERVAL_US);
        }

        // 버튼을 떼거나 녹음 시간이 다 찰 때까지 대기
        while (gpio_get_level(PIN_BTN) == 0 && is_recording == true) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // 녹음 중지
        esp_timer_stop(timer);
        is_recording = false;

        ESP_LOGI(TAG, "Recording Done. %d samples captured.", sample_index);
        ESP_LOGI(TAG, "Playing back on speaker...");

#if APPLY_MOVING_AVERAGE_FILTER
        ESP_LOGI(TAG, "Applying Moving Average filter...");
        if (sample_index > 1) {
            int16_t prev_sample = audio_buffer[0];
            for (int i = 1; i < sample_index; i++) {
                int16_t current_sample = audio_buffer[i];
                audio_buffer[i] = (prev_sample + current_sample) / 2;
                prev_sample = current_sample;
            }
        }
#endif
#if APPLY_FFT_NOTCH_FILTER
        // FFT는 샘플 수가 2의 거듭제곱일 때만 동작합니다.
        // 버퍼가 완전히 채워지지 않은 경우(사용자가 버튼을 일찍 뗀 경우) 필터를 건너뜁니다.
        if (sample_index != NUM_SAMPLES) {
            ESP_LOGW(TAG, "FFT filter skipped: buffer not full (%d/%d samples).", sample_index, NUM_SAMPLES);
        } else {
        ESP_LOGI(TAG, "Applying FFT Notch filter...");
        // FFT 연산을 위한 float 타입의 임시 버퍼를 동적 할당합니다.
        float *fft_buffer = (float *)malloc(sample_index * sizeof(float));
        if (fft_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for FFT buffer");
        } else {
            // 1. DC 오프셋을 먼저 제거하고, float으로 변환
            for (int i = 0; i < sample_index; i++) {
                fft_buffer[i] = (float)(audio_buffer[i] - 2048);
            }

            // 2. FFT 초기화
            if (dsps_fft2r_init_fc32(NULL, sample_index) != ESP_OK) {
                ESP_LOGE(TAG, "FFT init failed");
            } else {
                // 3. FFT 수행 (실수 -> 복소수)
                dsps_fft2r_fc32(fft_buffer, sample_index);

                // 4. 노치 필터 적용: 특정 주파수 대역의 값을 0으로 만듭니다.
                float freq_resolution = (float)SAMPLE_RATE / sample_index;
                int start_bin = (NOTCH_FILTER_FREQ - NOTCH_FILTER_WIDTH / 2) / freq_resolution;
                int end_bin = (NOTCH_FILTER_FREQ + NOTCH_FILTER_WIDTH / 2) / freq_resolution;

                if (start_bin < 0) start_bin = 0;
                if (end_bin >= sample_index / 2) end_bin = (sample_index / 2) - 1;

                for (int i = start_bin; i <= end_bin; i++) {
                    fft_buffer[i * 2] = 0;     // Real part
                    fft_buffer[i * 2 + 1] = 0; // Imaginary part
                }

                // 5. IFFT 수행: 복소 공액(Complex Conjugate)을 이용해 정방향 FFT 함수를 재사용합니다.
                // 5-1: 입력 데이터(주파수 영역)의 허수부 부호를 반전합니다.
                for (int i = 0; i < sample_index / 2; i++) {
                    fft_buffer[i * 2 + 1] *= -1;
                }
                // 5-2: 정방향 FFT 함수를 다시 실행합니다.
                dsps_fft2r_fc32(fft_buffer, sample_index);
                // 5-3: 결과 데이터(시간 영역)의 허수부 부호를 다시 반전합니다.
                for (int i = 0; i < sample_index / 2; i++) {
                    fft_buffer[i * 2 + 1] *= -1;
                }

                // 6. IFFT 결과를 다시 int16_t 오디오 데이터로 변환
                for (int i = 0; i < sample_index; i++) {
                    // 5-4: 결과를 N으로 나누어 스케일링하고 클리핑합니다.
                    float val = fft_buffer[i] / sample_index;
                    if (val > 2047) val = 2047;
                    if (val < -2048) val = -2048;
                    audio_buffer[i] = (int16_t)val;
                }
            }
            // 임시 버퍼 메모리 해제
            free(fft_buffer);
        }
        }
#endif

        // 💡 [핵심] 최종 데이터를 I2S 출력 형식으로 변환
        // AC 신호(-2048~2047)를 16비트 스케일로 증폭합니다.
        // FFT 필터를 사용하지 않은 경우, 이 단계에서 DC 오프셋이 제거됩니다.
        // 하나의 버퍼를 재사용하여 메모리를 절약합니다.
        for (int i = 0; i < sample_index; i++) {
#if APPLY_FFT_NOTCH_FILTER
            // FFT 필터를 거친 데이터는 이미 DC 오프셋이 제거되었으므로 바로 증폭합니다.
            audio_buffer[i] = audio_buffer[i] << 4;
#else
            // 원본 데이터는 여기서 DC 오프셋을 제거하고 증폭합니다.
            audio_buffer[i] = (audio_buffer[i] - 2048) << 4;
#endif
        }

        // 재생 직전에 I2S 채널을 활성화합니다.
        i2s_channel_enable(tx_handle);
        // I2S로 스피커에 데이터 쓰기
        size_t bytes_written = 0;
        i2s_channel_write(tx_handle, audio_buffer, sample_index * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        // 재생이 끝나면 I2S 채널을 비활성화하여 노이즈를 방지합니다.
        i2s_channel_disable(tx_handle);
        
        ESP_LOGI(TAG, "Playback Complete! Ready for next recording.");
        vTaskDelay(pdMS_TO_TICKS(500)); // 다음 녹음을 위한 약간의 딜레이
    }
}