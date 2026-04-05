#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define KEYPAD_ROWS     4
#define KEYPAD_COLS     4

#define KEYPAD_PIN_1    16
#define KEYPAD_PIN_2    17
#define KEYPAD_PIN_3    4
#define KEYPAD_PIN_4    23

#define KEYPAD_PIN_5    5
#define KEYPAD_PIN_6    18
#define KEYPAD_PIN_7    19
#define KEYPAD_PIN_8    2

const int GPIO_PULL_NONE = 0;
const int GPIO_PULL_UP = 1;
const int GPIO_PULL_DOWN = 2;

/*******************************************************************************
 * [ 4x4 Matrix Keypad Hardware Wiring Diagram ]
 *
 * Row 0 --[  1  ]-[  2  ]-[  3  ]-[  A  ]-- (P8)
 * Row 1 --[  4  ]-[  5  ]-[  6  ]-[  B  ]-- (P7)
 * Row 2 --[  7  ]-[  8  ]-[  9  ]-[  C  ]-- (P6)
 * Row 3 --[  *  ]-[  0  ]-[  #  ]-[  D  ]-- (P5)
 *            |       |       |       |
 *          Col 0   Col 1   Col 2   Col 3
 *          (P4)    (P3)    (P2)    (P1)
 *
 * [ Keypad Pinout (Left to Right) ]
 * Pin 8 : Row 0
 * Pin 7 : Row 1
 * Pin 6 : Row 2
 * Pin 5 : Row 3
 * Pin 4 : Col 0
 * Pin 3 : Col 1
 * Pin 2 : Col 2
 * Pin 1 : Col 3
 *
 * Note: Col pins are configured as INPUT with PULL-DOWN.
 * Row pins are configured as OUTPUT (High for scanning).
 ******************************************************************************/

const char letter[KEYPAD_ROWS][KEYPAD_COLS] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

const int keypad_row_pins[KEYPAD_ROWS] = {KEYPAD_PIN_8, KEYPAD_PIN_7, KEYPAD_PIN_6, KEYPAD_PIN_5};
const int keypad_col_pins[KEYPAD_COLS] = {KEYPAD_PIN_4, KEYPAD_PIN_3, KEYPAD_PIN_2, KEYPAD_PIN_1};

int keypad_input_init(int *_gpio_pins, int _max_pins, int _pull_type)
{
    int rst = 0;
    gpio_config_t io_conf = {};

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    
    //bit mask of the pins that you want to set
    uint64_t pin_mask = 0;
    for (int i = 0; i < _max_pins; i++)   pin_mask |= (1ULL << _gpio_pins[i]);
    io_conf.pin_bit_mask = pin_mask;
    
    //disable pull-up mode
    io_conf.pull_up_en = (_pull_type == GPIO_PULL_UP) ? 1 : GPIO_PULL_NONE;

    //disable pull-down mode
    io_conf.pull_down_en = (_pull_type == GPIO_PULL_DOWN) ? 1 : GPIO_PULL_NONE;
    
    //configure GPIO with the given settings
    rst = gpio_config(&io_conf);

    return rst;
}

int keypad_output_init(int *_gpio_pins, int _max_pins)
{
    int rst = 0;
    gpio_config_t io_conf = {};

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    
    //bit mask of the pins that you want to set
    uint64_t pin_mask = 0;
    for (int i = 0; i < _max_pins; i++)   pin_mask |= (1ULL << _gpio_pins[i]);
    io_conf.pin_bit_mask = pin_mask;
    
    //disable pull-up/pull-down mode
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    
    //configure GPIO with the given settings
    rst = gpio_config(&io_conf);

    if (rst == 0)   for (int i = 0; i < _max_pins; i++)   gpio_set_level(_gpio_pins[i], 0);

    return rst;
}

int keypad_init(int *_keypad_in_pins, int _max_in,
                int *_keypad_out_pins, int _max_out, int _pull_type)
{
    int rst1 = 0;
    int rst2 = 0;

    rst1 = keypad_input_init(_keypad_in_pins, _max_in, _pull_type);
    rst2 = keypad_output_init(_keypad_out_pins, _max_out);
    
    return (rst1 == 0) && (rst2 == 0) ? 0 : -1;
}

char keypad_getChar(int *_keypad_in_pins, int _max_in,
                    int *_keypad_out_pins, int _max_out,
                    int delay_ms)
{
    char rst = -1;

    for (int i = 0; i < _max_out; i++)
    {
        gpio_set_level(_keypad_out_pins[i], 1);
        ets_delay_us(10);

        for (int j = 0; j < _max_in; j++)
        {
            if (gpio_get_level(_keypad_in_pins[j]))
            {
                rst = letter[i][j];
            }
        }

        gpio_set_level(_keypad_out_pins[i], 0);
    }

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return rst;
}

void app_main(void)
{
    char keypad = 0;
    keypad_init(keypad_col_pins, KEYPAD_COLS, keypad_row_pins, KEYPAD_ROWS, GPIO_PULL_DOWN);
    while (1)
    {
        keypad = keypad_getChar(keypad_col_pins, KEYPAD_COLS, keypad_row_pins, KEYPAD_ROWS, 100);
        if (keypad != -1)
        {
            printf("Pressed : %c\n", keypad);
        }
    }
}