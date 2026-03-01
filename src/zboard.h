#ifndef _ZBOARD_H
#define _ZBOARD_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

// These #defines must occur before including led_map.h since they control the wiring configuration that the LED mapping code needs to know about

// Wiring must be zig-zag fashion up and down the columns.
// By default, we have the first LED at the top of the first column, and the columns go left to right
// The following defines can modify this
#define WIRING_FIRST_LED_RIGHT // Columns goes right to left instead
// #define WIRING_FIRST_LED_BOTTOM		1	// First LED is at the bottom of the first column instead of the top
//  If some LEDs need to be skipped because there is a beam or other obstruction that you need to go around/over,
//  you can control that here.You need to specify the LED numbers that will be skipped in the first column and the code
//  will assume the other columns are identical.
//  For example, if you have a beam between the 6th and 7th row, and another between the 12th and 13th row, and you need
//  to skip 2 LEDs for each beam, you'd specify 6,7,14,15 so that LEDs 0-5, 8-13, 16-21 are used for the first column
#define WIRING_LEDS_SKIP_FIRST_COLUMN 6, 7, 14, 15

#include "led_map.h"

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_LENGTH DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define LED_BRIGHTNESS 64
#define PROBLEM_STRING_MAX_LENGTH 256

#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}
#define COLOR(_r, _g, _b, _name) {.rgb = RGB((_r), (_g), (_b)), .name = (_name)}

#define LED_SET_ROW(row, color)              \
    for (int __i = 0; __i < NUM_COLS; __i++) \
    pixels[LED_MAP_COL_ROW(__i, row)] = (color).rgb

#define LED_SET_COL(col, color)              \
    for (int __i = 0; __i < NUM_ROWS; __i++) \
    pixels[LED_MAP_COL_ROW(col, __i)] = (color).rgb

typedef struct colorType
{
    struct led_rgb rgb;
    const char *name;
} color_t;

typedef enum parseState
{
    PARSE_START,
    PARSE_CONFIG,
    PARSE_PROB_START,
    PARSE_HOLDS
} parse_state_t;

extern struct led_rgb pixels[STRIP_LENGTH];
extern const struct device *const strip;

static const color_t color_list[] = {
    COLOR(LED_BRIGHTNESS, 0x00, 0x00, "red"),
    COLOR(0x00, LED_BRIGHTNESS, 0x00, "green"),
    COLOR(0x00, 0x00, LED_BRIGHTNESS, "blue"),
    COLOR(LED_BRIGHTNESS, LED_BRIGHTNESS, 0x00, "yellow"),
    COLOR(0x00, LED_BRIGHTNESS, LED_BRIGHTNESS, "cyan"),
    COLOR(LED_BRIGHTNESS, 0x00, LED_BRIGHTNESS / 2, "pink"),
    COLOR(LED_BRIGHTNESS / 2, 0x00, LED_BRIGHTNESS, "violet"),
    COLOR(LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS, "white"),
    COLOR(0x00, 0x00, 0x00, "black"),
};

#define COLOR_RED color_list[0]
#define COLOR_GREEN color_list[1]
#define COLOR_BLUE color_list[2]
#define COLOR_YELLOW color_list[3]
#define COLOR_CYAN color_list[4]
#define COLOR_PINK color_list[5]
#define COLOR_VIOLET color_list[6]
#define COLOR_WHITE color_list[7]
#define COLOR_BLACK color_list[8]

void clearStrip(bool updateStrip);

#endif // _ZBOARD_H