#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "lcd.h"

int main(void) {

    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    Lcd_init();  // lcdyi baþlatýr
    Lcd_Goto(1, 1);  // imleci o noktaya götür demektir
    Lcd_Puts("HATICE CELIK");
    Lcd_Goto(2, 6);
    Lcd_Puts("SAU");

    while (1) {}
}
