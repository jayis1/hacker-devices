/*
 * ssd1306.h — SSD1306 128x32 OLED display driver (SPI2)
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

/* Display dimensions */
#define SSD1306_WIDTH           128U
#define SSD1306_HEIGHT           32U
#define SSD1306_PAGES            4U   /* 32 pixels / 8 = 4 pages */

/* Font: 6x8 ASCII (95 printable chars 0x20-0x7E) */
#define FONT_WIDTH              6U
#define FONT_HEIGHT              8U

/* Function prototypes */
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_update(void);
void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void ssd1306_draw_char(uint8_t x, uint8_t page, char c);
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str);
void ssd1306_set_contrast(uint8_t contrast);
void ssd1306_display_on(void);
void ssd1306_display_off(void);

/* SPI2 CS control */
#define SSD1306_CS_LOW()   GPIOE->ODR &= ~(1U << OLED_CS_PIN)
#define SSD1306_CS_HIGH()  GPIOE->ODR |= (1U << OLED_CS_PIN)
#define SSD1306_DC_DATA()  GPIOB->ODR |= (1U << OLED_DC_PIN)
#define SSD1306_DC_CMD()   GPIOB->ODR &= ~(1U << OLED_DC_PIN)
#define SSD1306_RST_LOW()  GPIOB->ODR &= ~(1U << OLED_RST_PIN)
#define SSD1306_RST_HIGH() GPIOB->ODR |= (1U << OLED_RST_PIN)

#endif /* SSD1306_H */