#include "zboard.h"

#include <stdlib.h>

typedef void (*led_pattern_fn_t)(void);

void led_startup_pattern(void);
void left_to_right_pattern(void);
void right_to_left_pattern(void);
void top_to_bottom_pattern(void);
void bottom_to_top_pattern(void);

void show_random_pattern(void);
