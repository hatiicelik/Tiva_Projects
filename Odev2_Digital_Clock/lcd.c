#include <stdint.h>
#include <stdbool.h>
#include "lcd.h"

void Lcd_Komut(uint8_t c);
void Lcd_Putch(uint8_t d);
void Lcd_Temizle(void);
void Lcd_Puts(char* s);
void Lcd_Goto(char x, char y);

void Lcd_init(void) {

    SysCtlPeripheralEnable(LCDPORTENABLE);
    while(!SysCtlPeripheralReady(LCDPORTENABLE));
    GPIOPinTypeGPIOOutput(LCDPORT, RS | E | D4 | D5 | D6 | D7);

    SysCtlDelay(160000);   // güç sonrasý bekleme (~15ms)

    GPIOPinWrite(LCDPORT, RS, 0x00);

    // --- 4-bit moda geçiþ ---
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x30);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x30);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x30);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x20);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    // --- LCD ayar komutlarý ---
    Lcd_Komut(0x28);  // 4-bit, 2 satýr, 5x8 font
    Lcd_Komut(0x0C);  // Display ON, cursor OFF
    Lcd_Komut(0x06);  // Entry mode
    Lcd_Komut(0x01);  // Clear display
    SysCtlDelay(160000);
}

void Lcd_Komut(uint8_t c) {
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, (c & 0xF0));
    GPIOPinWrite(LCDPORT, RS, 0x00);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, (c & 0x0F) << 4);
    GPIOPinWrite(LCDPORT, RS, 0x00);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);
}

void Lcd_Putch(uint8_t d) {
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, (d & 0xF0));
    GPIOPinWrite(LCDPORT, RS, RS);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);

    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, (d & 0x0F) << 4);
    GPIOPinWrite(LCDPORT, RS, RS);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(10);
    GPIOPinWrite(LCDPORT, E, 0);
    SysCtlDelay(160000);
}

void Lcd_Temizle(void) {
    Lcd_Komut(0x01);
    SysCtlDelay(160000);
}

void Lcd_Goto(char x, char y) {
    if (x == 1)
        Lcd_Komut(0x80 + ((y - 1) % 16));
    else
        Lcd_Komut(0xC0 + ((y - 1) % 16));
}

void Lcd_Puts(char* s) {
    while (*s)
        Lcd_Putch(*s++);
}
