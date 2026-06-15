#ifndef REGISTERS_H
#define REGISTERS_H

#define REG_CTRL          0x00
#define REG_GLITCH_WIDTH  0x04
#define REG_GLITCH_DELAY  0x08
#define REG_TRIG_VAL      0x0C
#define REG_TRIG_MASK     0x10
#define REG_STATUS        0x14

/* CTRL Bits */
#define CTRL_ENABLE       (1 << 0)
#define CTRL_RESET        (1 << 1)
#define CTRL_ARM          (1 << 2)

/* STATUS Bits */
#define STATUS_TRIGGERED  (1 << 0)
#define STATUS_COMPLETE   (1 << 1)

#endif // REGISTERS_H
