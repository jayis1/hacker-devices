/* Hardware abstraction for STM32G474 and host verification.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "board.h"
#include "registers.h"
#include <stddef.h>
#ifdef HOST_SIM
#include <stdio.h>
static uint32_t simulated_time;
void board_init(void){simulated_time=0;}
uint32_t board_micros(void){return simulated_time+=100;}
void board_delay_us(uint32_t d){simulated_time+=d;}
void board_watchdog_kick(void){}
void board_set_bypass(bool transparent){(void)transparent;}
void board_set_transceiver(unsigned c,bool e){(void)c;(void)e;}
bool board_read_arm(void){return false;}
uint16_t board_target_mv(void){return 12000;}
int16_t board_target_ma(void){return 18;}
int board_uart_read(unsigned c,uint8_t *v){(void)c;(void)v;return 0;}
void board_uart_write(unsigned c,uint8_t v){(void)c;(void)v;}
int board_usb_read(uint8_t *d,unsigned c){(void)d;(void)c;return 0;}
void board_usb_write(const uint8_t *s,unsigned n){(void)s;(void)n;}
void board_led_pattern(unsigned p){(void)p;}
#else
static volatile uint32_t ticks;
void SysTick_Handler(void){ticks++;}
void board_init(void){
 RCC_AHB2ENR|=(1u<<0)|(1u<<1);RCC_APB1ENR1|=(1u<<17)|(1u<<18)|(1u<<0);
 /* PA2/PA3 AF7 USART2, PB10/PB11 AF7 USART3. */
 GPIO_MODER(GPIOA_BASE)=(GPIO_MODER(GPIOA_BASE)&~(0xfu<<4))|(0xau<<4);
 GPIO_MODER(GPIOB_BASE)=(GPIO_MODER(GPIOB_BASE)&~(0xfu<<20))|(0xau<<20);
 GPIO_AFRL(GPIOA_BASE)|=(7u<<8)|(7u<<12);
 USART_BRR(USART2_BASE)=CPU_HZ/LIN_UART_HZ;USART_BRR(USART3_BASE)=CPU_HZ/LIN_UART_HZ;
 USART_CR2(USART2_BASE)=USART_CR2_LINEN|USART_CR2_LBDL;
 USART_CR2(USART3_BASE)=USART_CR2_LINEN|USART_CR2_LBDL;
 USART_CR1(USART2_BASE)=USART_CR1_UE|USART_CR1_RE|USART_CR1_TE;
 USART_CR1(USART3_BASE)=USART_CR1_UE|USART_CR1_RE|USART_CR1_TE;
}
uint32_t board_micros(void){return ticks;}
void board_delay_us(uint32_t d){uint32_t s=board_micros();while((uint32_t)(board_micros()-s)<d){} }
void board_watchdog_kick(void){REG32(0x40003000u)=0xaaaau;}
void board_set_bypass(bool t){unsigned pin=PIN_RELAY_BYPASS_PB;REG32(GPIOB_BASE+0x18u)=1u<<(t?pin:pin+16u);}
void board_set_transceiver(unsigned c,bool e){unsigned pin=c?PIN_LIN_B_SLP_PB:PIN_LIN_A_SLP_PA;uint32_t base=c?GPIOB_BASE:GPIOA_BASE;REG32(base+0x18u)=1u<<(e?pin:pin+16u);}
bool board_read_arm(void){return (REG32(GPIOB_BASE+0x10u)&(1u<<PIN_ARM_BUTTON_PB))==0;}
uint16_t board_target_mv(void){return (uint16_t)((ADC_DR(ADC1_BASE)*24000u)/4095u);}
int16_t board_target_ma(void){return (int16_t)(((int32_t)ADC_DR(ADC1_BASE)-2048)*1000/2048);}
int board_uart_read(unsigned c,uint8_t *v){uint32_t b=c?USART3_BASE:USART2_BASE;if(USART_ISR(b)&USART_ISR_LBDF)return -1;if(!(USART_ISR(b)&USART_ISR_RXNE))return 0;*v=(uint8_t)USART_RDR(b);return 1;}
void board_uart_write(unsigned c,uint8_t v){uint32_t b=c?USART3_BASE:USART2_BASE;while(!(USART_ISR(b)&USART_ISR_TXE)){}USART_TDR(b)=v;}
/* USB CDC hooks are supplied by the STM32 USB device library in production. */
__attribute__((weak)) int board_usb_read(uint8_t *d,unsigned c){(void)d;(void)c;return 0;}
__attribute__((weak)) void board_usb_write(const uint8_t *s,unsigned n){(void)s;(void)n;}
void board_led_pattern(unsigned p){unsigned pin=PIN_STATUS_LED_PB;REG32(GPIOB_BASE+0x18u)=1u<<((p&1u)?pin:pin+16u);}
#endif
