#include <stdint.h>
#include <stdio.h>

#include "led_map.h"

int main(int argv, char *argc[])
{
    populate_led_map(led_map);
    
    printf("Printing zboard LED map:\n\n");
    for (int i = (NUM_COLS-1); i >= 0; i--) {
        int z = i * NUM_ROWS;
        printf("%3c: %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n", 'A' + i,
            led_map[z+17], led_map[z+16], led_map[z+15], led_map[z+14], led_map[z+13], led_map[z+12], led_map[z+11], led_map[z+10],led_map[z+9],
            led_map[z+8], led_map[z+7], led_map[z+6], led_map[z+5], led_map[z+4], led_map[z+3], led_map[z+2], led_map[z+1], led_map[z]);
    }
    printf("   |\n   |---18---17---16---15---14---13---12---11---10---9----8----7----6----5----4----3----2----1--|\n");
    return 0;
}