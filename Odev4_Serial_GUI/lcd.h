#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

// --- LCD pin tanýmlarý ---
#define LCDPORT           GPIO_PORTB_BASE
#define LCDPORTENABLE     SYSCTL_PERIPH_GPIOB
#define RS                GPIO_PIN_0
#define E                 GPIO_PIN_1
#define D4                GPIO_PIN_4
#define D5                GPIO_PIN_5
#define D6                GPIO_PIN_6
#define D7                GPIO_PIN_7

// --- Fonksiyon prototipleri ---
void Lcd_init(void);
void Lcd_Komut(uint8_t);
void Lcd_Temizle(void);
void Lcd_Puts(char*);
void Lcd_Goto(char, char);
void Lcd_Putch(uint8_t);

#endif /* LCD_H_ */
