#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* Pin Definitions */
#define GLITCH_EN_PORT  GPIOC
#define GLITCH_EN_PIN   GPIO_PIN_0

#define TRIGGER_IN_PORT GPIOE
#define TRIGGER_IN_PIN  GPIO_PIN_0

#define FPGA_CS_PORT    GPIOD
#define FPGA_CS_PIN     GPIO_PIN_0

/* FMC Base Address for FPGA Communication */
#define FPGA_BASE_ADDR  0x60000000

/* ADC Configuration */
#define ADC_SAMPLE_RATE 100000000 // 100 MSPS

#endif // BOARD_H
