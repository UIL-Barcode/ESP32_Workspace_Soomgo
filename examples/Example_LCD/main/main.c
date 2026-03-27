#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "rom/ets_sys.h"

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  50000
#define LCD_ADDR            0x27  // 스캐너로 확인한 주소 입력

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
    vTaskDelay(pdMS_TO_TICKS(50)); // 전원 인가 후 대기 (데이터시트: > 40ms)
    
    // 1. 하드웨어 리셋 1단계
    lcd_send_cmd(0x30);
    ets_delay_us(4500); // 데이터시트: > 4.1ms (4100us)
    
    // 2. 하드웨어 리셋 2단계
    lcd_send_cmd(0x30);
    ets_delay_us(150);  // 데이터시트: > 100us
    
    // 3. 하드웨어 리셋 3단계
    lcd_send_cmd(0x30);
    ets_delay_us(50);   // 데이터시트: > 37us
    
    // 4. 4-bit 인터페이스 설정
    lcd_send_cmd(0x20); 
    ets_delay_us(50);   // 데이터시트: > 37us
    
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
    while (*str) {
        lcd_send_data(*str++);
    }
}

/* -------------------------------------------------------------------------- */
/* 3. 메인 어플리케이션                                                       */
/* -------------------------------------------------------------------------- */
void app_main(void) {
    // 1. 초기화
    i2c_master_init();
    lcd_init();
    
    // 2. 출력 테스트
    lcd_put_cur(0, 0);
    lcd_send_string("ESP-IDF Ready");
    
    lcd_put_cur(1, 0);
    lcd_send_string("LCD I2C Test");

    // 3. 메인 루프 (대기)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}