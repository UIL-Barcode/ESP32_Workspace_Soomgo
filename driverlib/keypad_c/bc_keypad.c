#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "bc_keypad.h"

int keypad_input_init(const int *_gpio_pins, const int _max_pins, const int _pull_type)
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

int keypad_output_init(const int *_gpio_pins, const int _max_pins)
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

int keypad_init(const int *_keypad_in_pins, const int _max_in,
                const int *_keypad_out_pins, const int _max_out, const int _pull_type)
{
    int rst1 = 0;
    int rst2 = 0;

    rst1 = keypad_input_init(_keypad_in_pins, _max_in, _pull_type);
    rst2 = keypad_output_init(_keypad_out_pins, _max_out);
    
    return (rst1 == 0) && (rst2 == 0) ? 0 : -1;
}

char keypad_getChar(const int *_keypad_in_pins, const int _max_in,
                    const int *_keypad_out_pins, const int _max_out,
                    const char letter[_max_out][_max_in], const int delay_ms)
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
