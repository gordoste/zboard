#ifndef _LED_MAP_H
#define _LED_MAP_H

// Wiring must be zig-zag fashion up and down the columns.
// By default, we have the first LED at the top of the first column, and the columns go left to right
// Uncommenting the following defines can modify this
#define WIRING_FIRST_LED_RIGHT              // Columns goes right to left instead
// #define WIRING_FIRST_LED_BOTTOM			// First LED is at the bottom of the first column instead of the top

// If some LEDs need to be skipped because there is a beam or other obstruction that you need to go around/over,
// you can control that here. You need to specify the LED numbers that will be skipped in the first column and the code
// will assume the other columns are identical.
// For example, if you have a beam between the 6th and 7th row, and another between the 12th and 13th row, and you need
// to skip 2 LEDs for each beam, you'd specify 6,7,14,15 so that LEDs 0-5, 8-13, 16-21 are used for the first column
// #define WIRING_LEDS_SKIP_FIRST_COLUMN 6,7,14,15

// If you want to define your own wiring configuration, you need to unccmment the following line and implement initialize_led_map()
// in led_map.c
// #define WIRING_CUSTOM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NUM_ROWS 18
#define NUM_COLS 11
#define NUM_PIXELS NUM_ROWS *NUM_COLS

extern uint16_t led_map[NUM_PIXELS];

bool ledSkip(uint16_t);
void initialize_led_map();
uint16_t moonNumToMapNum(uint16_t moonNum);

#endif // _LED_MAP_H