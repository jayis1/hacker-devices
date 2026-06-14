/*
 * i2c_eeprom.h — AT24C02 I2C EEPROM driver
 */

#ifndef I2C_EEPROM_DRIVER_H
#define I2C_EEPROM_DRIVER_H

#include <stdint.h>

#define EEPROM_I2C_ADDR   0x50  /* 7-bit address */
#define EEPROM_PAGE_SIZE  8     /* 8-byte pages */
#define EEPROM_SIZE       256   /* 2 Kb = 256 bytes */

int  i2c_eeprom_init(void);
int  i2c_eeprom_write(uint8_t dev_addr, uint16_t mem_addr, const uint8_t *data, uint16_t len);
int  i2c_eeprom_read(uint8_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t len);

#endif /* I2C_EEPROM_DRIVER_H */