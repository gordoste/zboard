#include "zboard.h"

void led_startup_pattern(void);

typedef void (*led_pattern_fn_t)(void);

void show_random_pattern(void);
