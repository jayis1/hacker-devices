#ifndef GLITCH_H
#define GLITCH_H

#include <stdint.h>

typedef struct {
    uint32_t width_ns;
    uint32_t delay_ns;
    uint16_t trigger_val;
    uint16_t trigger_mask;
} glitch_config_t;

void glitch_init(void);
void glitch_configure(glitch_config_t *config);
void glitch_arm(void);
void glitch_manual_fire(void);
int glitch_is_complete(void);

#endif // GLITCH_H
