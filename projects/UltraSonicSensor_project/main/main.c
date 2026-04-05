#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "rom/ets_sys.h"
#include <string.h> // strcmp, strcpy 사용을 위해 필요
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h" // 정밀 타이머 사용을 위한 필수 헤더
#include "rom/ets_sys.h" // esp_rom_delay_us 사용을 위해 필요


#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  50000
#define LCD_ADDR            0x27  // 스캐너로 확인한 주소 입력

#define TRIG_PIN 14 //초음파 센서 트리거 핀
#define ECHO_PIN 27 //초음파 센서 에코 핀 (1k/2k 전압 변환 회로로 연결)

/* -------------------------------------------------------------------------- */
/* 1. I2C 하드웨어 초기화                                                     */
/* -------------------------------------------------------------------------- */
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/* -------------------------------------------------------------------------- */
/* 2. LCD 제어 함수 (명령어 / 데이터 / 커서 / 문자열)                           */
/* -------------------------------------------------------------------------- */
void lcd_send_init_nibble(char nibble) {
    uint8_t data_t[2];
    // 상위 4비트 데이터에 설정값 결합
    char data = (nibble & 0xF0);
    
    // 0x0C = Backlight(1), EN(1), RW(0), RS(0)
    // 0x08 = Backlight(1), EN(0), RW(0), RS(0)
    data_t[0] = data | 0x0C; // Pulse (EN=1)
    data_t[1] = data | 0x08; // Hold  (EN=0)
    
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, data_t, 2, pdMS_TO_TICKS(100));
    ets_delay_us(50); //50 us 동안 대기, LCD 데이터시트에 따르면 초기화 시퀀스에서는 40us 이상 대기해야 함. 안전하게 50us로 설정.
}

// LCD에 명령어(Command) 전송 (RS = 0)
void lcd_send_cmd(char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (cmd & 0xF0);        // 상위 4비트
    data_l = ((cmd << 4) & 0xF0); // 하위 4비트
    
    // 0x0C = Backlight(1), EN(1), RW(0), RS(0)
    // 0x08 = Backlight(1), EN(0), RW(0), RS(0)
    data_t[0] = data_u | 0x0C; // 상위 데이터 + EN High
    data_t[1] = data_u | 0x08; // 상위 데이터 + EN Low (래치)
    data_t[2] = data_l | 0x0C; // 하위 데이터 + EN High
    data_t[3] = data_l | 0x08; // 하위 데이터 + EN Low (래치)
    
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, data_t, 4, pdMS_TO_TICKS(100));
    ets_delay_us(50); // 명령어가 물리적으로 처리될 시간 대기
}

// LCD에 화면에 보일 글자(Data) 전송 (RS = 1)
void lcd_send_data(char data) {
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (data & 0xF0);
    data_l = ((data << 4) & 0xF0);
    
    // 0x0D = Backlight(1), EN(1), RW(0), RS(1)
    // 0x09 = Backlight(1), EN(0), RW(0), RS(1)
    data_t[0] = data_u | 0x0D; 
    data_t[1] = data_u | 0x09; 
    data_t[2] = data_l | 0x0D; 
    data_t[3] = data_l | 0x09; 
    
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, data_t, 4, pdMS_TO_TICKS(100));
    ets_delay_us(50); 
}

// 초기화 시퀀스 (데이터시트 극한 최적화)
void lcd_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50)); // 전원 인가 안정화 대기
    
    // 강제 하드웨어 리셋 시퀀스 (반드시 단일 니블로 전송)
    // 현재 꼬여있는 4-bit 찌꺼기 상태를 강제로 8-bit 모드로 덮어씀
    lcd_send_init_nibble(0x30); 
    ets_delay_us(4500);         // > 4.1ms
    
    lcd_send_init_nibble(0x30); 
    ets_delay_us(150);          // > 100us
    
    lcd_send_init_nibble(0x30); 
    ets_delay_us(50);
    
    // 8-bit 모드에서 4-bit 모드로 전환 명령
    lcd_send_init_nibble(0x20); 
    ets_delay_us(50);
    
    // 5. 디스플레이 설정 (이하 일반 명령어는 모두 37us 소요)
    lcd_send_cmd(0x28); // 2 Lines, 5x8 Matrix
    ets_delay_us(50);
    
    lcd_send_cmd(0x0C); // Display ON, Cursor OFF
    ets_delay_us(50);
    
    // 6. 화면 지우기 (가장 무거운 작업)
    lcd_send_cmd(0x01); 
    ets_delay_us(2000); // 데이터시트: > 1.52ms (1520us). 1000us는 위험하므로 2000us로 늘림.
}

// 커서 위치 이동
void lcd_put_cur(int row, int col) {
    switch (row) {
        case 0: col |= 0x80; break;
        case 1: col |= 0xC0; break;
    }
    lcd_send_cmd(col);
}

// 문자열 출력
void lcd_send_string(char *str) {
    //C언어에서 모든 문자열의 끝에는 우리 눈에 보이지 않는 **\0 (NULL 문자, 숫자 0)**이 숨어 있습니다. while 문은 0이 들어오면 거짓(False)으로 판단하여 멈추기 때문에, 문자열이 끝날 때까지 정확히 반복하게 됩니다.
    while (*str) {
        lcd_send_data(*str++);
    }
}


/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output (ESP32C2/ESP32H2 uses GPIO8 as the second output pin)
 * GPIO19: output (ESP32C2/ESP32H2 uses GPIO9 as the second output pin)
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Note. These are the default GPIO pins to be used in the example. You can
 * change IO pins in menuconfig.
 *
 * Test:
 * Connect GPIO18(8) with GPIO4
 * Connect GPIO19(9) with GPIO5
 * Generate pulses on GPIO18(8)/19(9), that triggers interrupt on GPIO4/5
 *
 */


#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_IO_1    18
#define GPIO_OUTPUT_IO_2    19
#define GPIO_OUTPUT_IO_3    2
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1)| (1ULL<<GPIO_OUTPUT_IO_2)| (1ULL<<GPIO_OUTPUT_IO_3))
/*
 * Let's say, GPIO_OUTPUT_IO_0=18, GPIO_OUTPUT_IO_1=19
 * In binary representation,
 * 1ULL<<GPIO_OUTPUT_IO_0 is equal to 0000000000000000000001000000000000000000 and
 * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
 * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
 * */

#define GPIO_INPUT_IO_0     16
#define GPIO_INPUT_IO_1     17
#define GPIO_INPUT_IO_2     4
#define GPIO_INPUT_IO_3     23
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1)| (1ULL<<GPIO_INPUT_IO_2)| (1ULL<<GPIO_INPUT_IO_3))
/*
 * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
 * In binary representation,
 * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
 * 1ULL<<GPIO_INPUT_IO_1 is equal to 00000000000000₀₀₀₀₀₀₀₀₀₀₀₀₀₀₁₀₀₀₀
 * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
 * */
char data[4][4] = 
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char text[2][17] = {0}; // 하나의 줄 마지막에 null이 필요하기 때문에 17로 설정

/* -------------------------------------------------------------------------- */
/* 0. 회원 데이터베이스 구조체                                                    */
/* -------------------------------------------------------------------------- */
// 회원 데이터베이스 구조체 (최대 10명)
typedef struct {
    char birth[7]; // 생년월일 6자리 + NULL
    char pw[5];    // 비밀번호 4자리 + NULL
    bool is_active;
} UserDB;

UserDB users[10];
int user_cnt = 0;

// 화면을 깨끗하게 지우는 편의 함수
void lcd_clear() {
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2)); // 화면 지우기는 시간이 좀 더 필요함
}

// 키패드 입력을 받아 buffer에 저장. (#버튼 = 입력 완료(Enter), B버튼 = 지우기)
void get_keypad_input(char* buffer, int max_len, bool is_password) {
    int idx = 0;
    char last_key = '\0';
    
    while (1) {
        char current_key = '\0';
        for (int i = 0; i < 4; i++) {
            gpio_set_level(GPIO_OUTPUT_IO_0, i == 0);
            gpio_set_level(GPIO_OUTPUT_IO_1, i == 1);
            gpio_set_level(GPIO_OUTPUT_IO_2, i == 2);
            gpio_set_level(GPIO_OUTPUT_IO_3, i == 3);
            
            if (gpio_get_level(GPIO_INPUT_IO_0)) current_key = data[3][3-i];
            if (gpio_get_level(GPIO_INPUT_IO_1)) current_key = data[2][3-i];
            if (gpio_get_level(GPIO_INPUT_IO_2)) current_key = data[1][3-i];
            if (gpio_get_level(GPIO_INPUT_IO_3)) current_key = data[0][3-i];
        }
        
        // 버튼이 새로 눌렸을 때만 반응 (엣지 디텍션)
        if (current_key != '\0' && current_key != last_key) {
            if (current_key == '#') { // '#'를 Enter 키로 활용
                if (idx > 0) break;   // 하나라도 입력되어야 넘어감
            } 
            else if (current_key == 'B') { // 'B'를 백스페이스로 활용
                if (idx > 0) {
                    idx--;
                    buffer[idx] = '\0';
                    lcd_put_cur(1, idx);
                    lcd_send_data(' '); // 화면에서 글자 지우기
                    lcd_put_cur(1, idx); // 커서 원위치
                }
            } 
            else if (idx < max_len && current_key >= '0' && current_key <= '9') {
                buffer[idx] = current_key;
                lcd_put_cur(1, idx);
                
                // 비밀번호 모드면 '*' 출력, 아니면 입력한 숫자 출력
                if (is_password) lcd_send_data('*');
                else lcd_send_data(current_key);
                
                idx++;
            }
        }
        last_key = current_key;
        vTaskDelay(pdMS_TO_TICKS(50)); // 채터링 방지 딜레이
    }
    buffer[idx] = '\0'; // 문자열 완성
}


// ADC 핸들 선언 (전역 변수)
adc_oneshot_unit_handle_t adc2_handle;

void adc_init(void) {
    // 1. ADC 유닛 2 초기화 (GPIO 13이 포함된 그룹)
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc2_handle));

    // 2. ADC 채널 4 (GPIO 13) 설정
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12비트 해상도 (0 ~ 4095)
        .atten = ADC_ATTEN_DB_12,         // 12dB 감쇄 (약 0V ~ 3.3V 범위 측정용)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_4, &config));
}


// 초음파 센서 GPIO 초기화 함수
void hcsr04_init(void) {
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&trig_conf);

    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&echo_conf);
    
    gpio_set_level(TRIG_PIN, 0); // 평상시 TRIG는 Low 유지
}

// 거리 측정 함수 (cm 단위 반환)
float get_distance(void) {
    // 1. 센서에 10us 동안 High 신호를 주어 초음파 발사 명령 (Trigger)
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // 2. Timeout 설정 (허공을 향해 쏘면 무한정 대기하는 버그 방지용)
    int64_t start_time = esp_timer_get_time();
    int64_t timeout = start_time + 30000; // 30ms 타임아웃 (약 5미터 왕복 시간)

    // 3. ECHO 핀이 High가 될 때까지(소리가 발사될 때까지) 대기
    while(gpio_get_level(ECHO_PIN) == 0 && esp_timer_get_time() < timeout) {}
    int64_t echo_start = esp_timer_get_time(); // 발사된 시간 기록

    // 4. ECHO 핀이 Low가 될 때까지(반사파가 돌아올 때까지) 대기
    while(gpio_get_level(ECHO_PIN) == 1 && esp_timer_get_time() < timeout) {}
    int64_t echo_end = esp_timer_get_time(); // 돌아온 시간 기록

    // 타임아웃 발생 시 측정 실패(-1) 반환
    if (echo_end >= timeout) return -1.0;

    // 5. 거리 계산: (왕복 시간(us) * 소리 속도(340m/s = 0.0343cm/us)) / 2
    int64_t time_diff = echo_end - echo_start;
    return (float)time_diff * 0.0343 / 2.0;
}


//Moving Average Filter (이동 평균 필터) 구현
#define WINDOW_SIZE 5 // 평균을 낼 데이터의 개수 (값을 키울수록 부드러워지지만, 반응 속도는 느려짐)

float dist_window[WINDOW_SIZE] = {0,};
int window_idx = 0;
bool is_window_filled = false;

// 이동 평균 필터 함수
float get_filtered_distance(float new_dist) {
    // 1. 예외 처리: 센서 타임아웃 등 에러 값(-1.0)이 들어오면 버퍼에 넣지 않고 그대로 통과
    if (new_dist < 0) {
        return -1.0; 
    }

    // 2. 새로운 값을 버퍼의 현재 인덱스 자리에 덮어쓰기
    dist_window[window_idx] = new_dist;
    window_idx++;

    // 3. 인덱스가 창 크기를 넘어가면 다시 처음(0)으로 되돌림 (원형 버퍼 회전)
    if (window_idx >= WINDOW_SIZE) {
        window_idx = 0;
        is_window_filled = true;
    }

    // 4. 현재까지 버퍼에 쌓인 유효한 데이터의 개수 파악
    int count = is_window_filled ? WINDOW_SIZE : window_idx;
    
    // 5. 버퍼 안의 데이터 합산 및 평균 계산
    float sum = 0;
    for (int i = 0; i < count; i++) {
        sum += dist_window[i];
    }

    return sum / count;
}

void app_main(void)
{
    // 1. 초기화
    i2c_master_init();
    lcd_init();
    
    // 2. 출력 테스트
    lcd_put_cur(0, 0);
    lcd_send_string("ESP-IDF Ready");
    
    lcd_put_cur(1, 0);
    lcd_send_string("LCD I2C Test");

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
 //   io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-down mode
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
  
    // gpio_set_level(GPIO_OUTPUT_IO_0, 1);
    // gpio_set_level(GPIO_OUTPUT_IO_1, 0);
    // gpio_set_level(GPIO_OUTPUT_IO_2, 0);
    // gpio_set_level(GPIO_OUTPUT_IO_3, 0);

    int32_t rows[4][4] = {0};
  //int txt_cnt = 0;


    // GPIO13 ADC 입력 발전기 전압 모니터링
    // ... 기존 GPIO 및 I2C 초기화 코드 ...
    // ... 기존 I2C 및 LCD 초기화 코드 유지 ...
    
    // ADC 하드웨어 초기화 실행
    // adc_init();

    // 초음파 센서 초기화
    hcsr04_init();

   // ... 기존 초기화 코드 유지 ...

    while (1) {
        // 1. 센서에서 원시 데이터(Raw Data)를 읽어옴
        float raw_distance = get_distance();
        
        // 2. 원시 데이터를 이동 평균 필터에 통과시킴
        float avg_distance = get_filtered_distance(raw_distance);

        char dist_str[16];
        if (avg_distance < 0) {
            sprintf(dist_str, "Out of Range  ");
        } else {
            // 원시 값 대신 필터링된 평균값을 소수점 첫째 자리까지 출력
            sprintf(dist_str, "Avg: %5.1f cm", avg_distance); 
        }

        // 3. LCD 출력
        lcd_clear();
        lcd_put_cur(0, 0);
        lcd_send_string("Moving Average"); // 첫 줄 안내문 변경
        lcd_put_cur(1, 0);
        lcd_send_string(dist_str);

        vTaskDelay(pdMS_TO_TICKS(100)); // 측정 주기 (너무 빠르면 초음파 간섭 발생)
    }
}
