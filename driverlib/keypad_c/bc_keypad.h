#pragma once

#ifndef BC_KEYPAD
#define BC_KEYPAD

#define GPIO_PULL_NONE  0
#define GPIO_PULL_UP    1
#define GPIO_PULL_DOWN  2

int keypad_input_init(const int* _gpio_pins, const int _max_pins, const int _pull_type);
int keypad_output_init(const int* _gpio_pins, const int _max_pins);
int keypad_init(const int* _keypad_in_pins, const int _max_in,
    const int* _keypad_out_pins, const int _max_out, const int _pull_type);
char keypad_getChar(const int* _keypad_in_pins, const int _max_in,
    const int* _keypad_out_pins, const int _max_out, 
    const char letter[_max_out][_max_in], const int delay_ms);

#endif
