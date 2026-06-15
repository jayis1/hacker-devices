#include "glitch.h"
#include "fpga.h"
#include "../registers.h"
#include "../board.h"

void glitch_init(void) {
    fpga_init();
    fpga_write_reg(REG_CTRL, CTRL_RESET);
}

void glitch_configure(glitch_config_t *config) {
    fpga_write_reg(REG_GLITCH_WIDTH, config->width_ns);
    fpga_write_reg(REG_GLITCH_DELAY, config->delay_ns);
    fpga_write_reg(REG_TRIG_VAL, config->trigger_val);
    fpga_write_reg(REG_TRIG_MASK, config->trigger_mask);
}

void glitch_arm(void) {
    fpga_write_reg(REG_CTRL, CTRL_ENABLE | CTRL_ARM);
}

void glitch_manual_fire(void) {
    // Manually pulse the gate driver via MCU GPIO for long glitches
    // HAL_GPIO_WritePin(GLITCH_EN_PORT, GLITCH_EN_PIN, GPIO_PIN_SET);
    // delay...
    // HAL_GPIO_WritePin(GLITCH_EN_PORT, GLITCH_EN_PIN, GPIO_PIN_RESET);
}

int glitch_is_complete(void) {
    return (fpga_read_reg(REG_STATUS) & STATUS_COMPLETE);
}
