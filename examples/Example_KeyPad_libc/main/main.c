#include <stdio.h>
#include "bc_keypad.h"

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

void app_main(void)
{
    char keypad = 0;
    keypad_init(keypad_col_pins, KEYPAD_COLS, keypad_row_pins, KEYPAD_ROWS, GPIO_PULL_DOWN);
    while (1)
    {
        keypad = keypad_getChar(keypad_col_pins, KEYPAD_COLS, keypad_row_pins, KEYPAD_ROWS, letter, 100);
        if (keypad != -1)
        {
            printf("Pressed : %c\n", keypad);
        }
    }
}