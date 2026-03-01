#include "led_patterns.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(led_patterns);

led_pattern_fn_t led_patterns[] = {
    led_startup_pattern,
    left_to_right_pattern,
    right_to_left_pattern,
    top_to_bottom_pattern,
    bottom_to_top_pattern};

#define NUM_LED_PATTERNS (sizeof(led_patterns) / sizeof(led_patterns[0]))

void led_startup_pattern(void)
{
    for (int c = 0; c < 40; c++) // flash all LEDs in sequence to show we're alive and have Bluetooth working
    {
        for (int r = 0; r < NUM_ROWS; r++)
        {
            LED_SET_ROW(r, color_list[(r + c) % 8]);
        }
        int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
        if (err)
        {
            LOG_ERR("Failed to update LED strip: %d", err);
            return;
        }
        k_sleep(K_MSEC(100));
    }
}

void left_to_right_pattern(void)
{
    for (int c = 0; c < NUM_COLS; c++)
    {
        clearStrip(false);
        for (int r = 0; r < NUM_ROWS; r++) {
            LED_SET_PIXEL(c, r, color_list[r % NUM_COLORS]);
        }
        int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
        if (err)
        {
            LOG_ERR("Failed to update LED strip: %d", err);
            return;
        }
        k_sleep(K_MSEC(100));
    }
}

void right_to_left_pattern(void)
{
    for (int c = NUM_COLS - 1; c >= 0; c--)
    {
        clearStrip(false);
        for (int r = 0; r < NUM_ROWS; r++) {
            LED_SET_PIXEL(c, r, color_list[r % NUM_COLORS]);
        }
        int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
        if (err)
        {
            LOG_ERR("Failed to update LED strip: %d", err);
            return;
        }
        k_sleep(K_MSEC(100));
    }
}

void top_to_bottom_pattern(void)
{
    for (int r = NUM_ROWS - 1; r >= 0; r--)
    {
        clearStrip(false);
        for (int c = 0; c < NUM_COLS; c++) {
            LED_SET_PIXEL(c, r, color_list[c % NUM_COLORS]);
        }
        int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
        if (err)
        {
            LOG_ERR("Failed to update LED strip: %d", err);
            return;
        }
        k_sleep(K_MSEC(100));
    }
}

void bottom_to_top_pattern(void)
{
    for (int r = 0; r < NUM_ROWS; r++)
    {
        clearStrip(false);
        for (int c = 0; c < NUM_COLS; c++) {
            LED_SET_PIXEL(c, r, color_list[c % NUM_COLORS]);
        }
        int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
        if (err)
        {
            LOG_ERR("Failed to update LED strip: %d", err);
            return;
        }
        k_sleep(K_MSEC(100));
    }
}

void show_random_pattern(void)
{
    int pattern_index = rand() % NUM_LED_PATTERNS;
    led_patterns[pattern_index]();
}