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
#include "inc/hw_ints.h"
#include "lcd.h"   // LCD
#include "inc/hw_uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"



void initmikro(void);
void timerkesme(void);
void ADC0_Init(void);
uint32_t ReadADC(void);
void UART0_Init(void);
void UART_ReadTimeFromPC(void);

int sn, dk, sa;

int main(void)
{
    initmikro();     // Saat, Timer0 vs. ayarlarý (TimerEnable artýk burada deðil)
    Lcd_init();      // LCD’yi baþlat
    ADC0_Init();     // ADC0 ve PE3 (pot) baþlat
    UART0_Init();    // UART0 baþlat (PC ile haberleþme)



    // PC'den zamaný al (bloklar, yani kullanýcý zaman gönderene kadar bekler)
    UART_ReadTimeFromPC();

    // Zamaný aldýktan sonra Timer'i baþlat -> timerkesme() artýk bu saatten itibaren SAYACAK
    TimerEnable(TIMER0_BASE, TIMER_A);

    while(1){
        // Tüm iþler timerkesme() içinde yapýlýyor (saat + ADC gösterimi)
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

    //TimerEnable(TIMER0_BASE, TIMER_A);
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

void UART0_Init(void)
{
    // UART0 için GPIOA ve UART0 modüllerini aç
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    // PA0 -> U0RX, PA1 -> U0TX
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Sistem frekansý: SysCtlClockSet ile 40 MHz ayarlamýþtýk
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    UARTEnable(UART0_BASE);
}

extern int sn, dk, sa;   // main.c'nin baþýnda zaten global olarak tanýmlý

void UART_ReadTimeFromPC(void)
{
    char buf[9];
    int i;
    int hour, min, sec;


    for(i = 0; i < 8; i++)
    {
        buf[i] = UARTCharGet(UART0_BASE);
    }
    buf[8] = '\0';

    // buf = "HH:MM:SS" formatýnda
    hour = (buf[0] - '0') * 10 + (buf[1] - '0');
    min  = (buf[3] - '0') * 10 + (buf[4] - '0');
    sec  = (buf[6] - '0') * 10 + (buf[7] - '0');

    sa = hour;
    dk = min;
    sn = sec;

    // LCD'de hemen göster
    Lcd_Goto(2, 1);
    Lcd_Putch('0' + (sa / 10));
    Lcd_Putch('0' + (sa % 10));
    Lcd_Putch(':');
    Lcd_Putch('0' + (dk / 10));
    Lcd_Putch('0' + (dk % 10));
    Lcd_Putch(':');
    Lcd_Putch('0' + (sn / 10));
    Lcd_Putch('0' + (sn % 10));
}


void timerkesme(void){
    char buf[6];
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


    Lcd_Goto(2, 1);

    Lcd_Putch('0' + (sa / 10));   // onlar
    Lcd_Putch('0' + (sa % 10));   // birler
    Lcd_Putch(':');

    Lcd_Putch('0' + (dk / 10));
    Lcd_Putch('0' + (dk % 10));
    Lcd_Putch(':');

    Lcd_Putch('0' + (sn / 10));
    Lcd_Putch('0' + (sn % 10));


    adcValue = ReadADC();


    Lcd_Goto(2, 11);
    sprintf(buf, "%4lu", (unsigned long)adcValue);
    Lcd_Puts(buf);


    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
                 ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1));

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}
