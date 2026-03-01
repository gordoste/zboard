#include "led_patterns.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(led_patterns);

led_pattern_fn_t led_patterns[] = {
    led_startup_pattern};

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
        }
        k_sleep(K_MSEC(100));
    }
}

void show_random_pattern(void)
{
}