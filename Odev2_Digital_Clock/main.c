#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_gpio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"     // <-- ADC için eklendi

#include "lcd.h"   // LCD

void initmikro(void);
void timerkesme(void);
void ADC0_Init(void);
uint32_t ReadADC(void);

int sn, dk, sa;

int main(void)
{
    initmikro();
    Lcd_init();     // LCD’yi baþlat
    ADC0_Init();    // ADC0 ve PE3 (pot) baþlat

    // Ýstersen ekrana sabit bir þey yazabilirsin (zorunlu deðil):
    // Lcd_Goto(1, 1);
    // Lcd_Puts("ADC:");

    while(1){
        // Tüm iþler timer kesmesinde yapýlýyor (saat + ADC gösterimi)
        // Ana döngü boþ kalabilir
    }
}

void initmikro(void){
    sn = 0;
    dk = 0;
    sa = 0;

    // 400/2/5 = 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 40000000 - 1);  // 1 saniye

    IntMasterEnable();           // global
    IntEnable(INT_TIMER0A);      // global
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);   // lokal

    TimerIntRegister(TIMER0_BASE, TIMER_A, timerkesme);
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER0_BASE, TIMER_A);
}

// ---------- ADC0 baþlatma (PE3 / AIN0) ----------
void ADC0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    // PE3'ü ADC giriþ pini yap (AIN0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    // ADC0, Sequence 3 (tek örnek)
    ADCSequenceDisable(ADC0_BASE, 3);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                             ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);
}

// ---------- ADC'den tek örnek okuma (0–4095) ----------
uint32_t ReadADC(void)
{
    uint32_t value;

    ADCIntClear(ADC0_BASE, 3);
    ADCProcessorTrigger(ADC0_BASE, 3);

    while(!ADCIntStatus(ADC0_BASE, 3, false));

    ADCSequenceDataGet(ADC0_BASE, 3, &value);

    return value;
}

// ---------- Timer0 kesmesi: saat + ADC deðeri ----------
void timerkesme(void){
    char buf[6];         // "4095\0" için yeter
    uint32_t adcValue;

    sn++;
    if(sn == 60){
        sn = 0;
        dk++;
        if(dk == 60){
            dk = 0;
            sa++;
            if(sa == 24){
                sa = 0;
            }
        }
    }

    // 2. satýr baþýna saat yaz (hh:mm:ss)
    Lcd_Goto(2, 1);

    Lcd_Putch('0' + (sa / 10));   // onlar
    Lcd_Putch('0' + (sa % 10));   // birler
    Lcd_Putch(':');

    Lcd_Putch('0' + (dk / 10));
    Lcd_Putch('0' + (dk % 10));
    Lcd_Putch(':');

    Lcd_Putch('0' + (sn / 10));
    Lcd_Putch('0' + (sn % 10));

    // Pot deðerini oku (0–4095)
    adcValue = ReadADC();

    // 2. satýr sað tarafa yaz (örn. kolon 11–14 arasý)
    // 16x2 LCD varsayýyorum: "hh:mm:ss" 8 karakter, biraz boþluk býrakýyoruz
    Lcd_Goto(2, 11);
    sprintf(buf, "%4lu", (unsigned long)adcValue);  // " 123" gibi
    Lcd_Puts(buf);

    // LED toggle (PF1)
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
                 ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1));

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}
