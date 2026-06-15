#include <stdio.h>
#include "board.h"
#include "drivers/fpga.h"
#include "drivers/glitch.h"

int main(void) {
    // HAL_Init();
    // SystemClock_Config();
    
    glitch_init();
    
    printf("Volt-Glitcher Initialized\n");
    
    glitch_config_t config = {
        .width_ns = 50,
        .delay_ns = 1000,
        .trigger_val = 0xABCD,
        .trigger_mask = 0xFFFF
    };
    
    glitch_configure(&config);
    glitch_arm();
    
    while (1) {
        if (glitch_is_complete()) {
            printf("Glitch Successful!\n");
            // Rearm or process trace data
            glitch_arm();
        }
    }
    
    return 0;
}
