#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22


void app_main(void)
{
    esp_err_t ret;

    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
//        .miso_io_num = PIN_NUM_MISO,
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
        .queue_size = 7                        //We want to be able to queue 7 transactions at a time
//        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };

        //Initialize the SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

//   sendCommandAll(0x0C, 0x01);  // Exit shutdown mode
//   sendCommandAll(0x09, 0x00);  // Disable decode mode
//   sendCommandAll(0x0A, 0x0F);  // Set intensity (0x00 to 0x0F)
//   sendCommandAll(0x0B, 0x07);  // Set scan limit (0 to 7)
//   sendCommandAll(0x0F, 0x00);  // Disable display test mode

      char cmd = 0x0C;

    spi_transaction_t t= {
    0};
//    memset(&t, 0, sizeof(t));       //Zero out the transaction, 메모리를 모두 0으로 넣어주는 작업
    t.length = 1;                   //Command is 8 bits
    t.tx_buffer = &cmd;             //The data is the cmd itself
    t.user = (void*)0;              //D/C needs to be set to 0
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    cmd = 0x01; // Data byte to turn on the display
    ret = spi_device_polling_transmit(spi, &t); //Transmit!

}
