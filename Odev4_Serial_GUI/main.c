#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

#include "lcd.h"

/* PC -> Tiva:
     ?HH:MM:SS\n
     *ABC\n
   Tiva -> PC (1Hz):
     RHH:MM:SS,ADC,PF4\n
*/

#define UART_BAUDRATE 9600
#define RX_LINE_MAX   32

volatile uint8_t g_h = 0, g_m = 0, g_s = 0;
volatile bool g_sendFlag = false;
volatile bool g_lcdFlag  = false;

static char g_text3[4] = "   ";

static volatile char g_rxLine[RX_LINE_MAX];
static volatile uint32_t g_rxIdx = 0;

/* ---------- UART TX ---------- */
static void UART0_SendChar(char c)
{
    UARTCharPut(UART0_BASE, c);
}
static void UART0_SendString(const char *s)
{
    while (*s) { UART0_SendChar(*s); s++; }
}

/* ---------- TIME ---------- */
static void FormatTime(char *out, size_t outsz, uint8_t hh, uint8_t mm, uint8_t ss)
{
    (void)snprintf(out, outsz, "%02u:%02u:%02u", (unsigned)hh, (unsigned)mm, (unsigned)ss);
}
static void ApplyTime(uint8_t hh, uint8_t mm, uint8_t ss)
{
    if (hh < 24 && mm < 60 && ss < 60)
    {
        g_h = hh; g_m = mm; g_s = ss;
        g_lcdFlag = true;
    }
}
static void ApplyText3(const char *t)
{
    g_text3[0] = t[0];
    g_text3[1] = t[1];
    g_text3[2] = t[2];
    g_text3[3] = '\0';
    g_lcdFlag = true;
}

/* ---------- LCD (SADECE DEÐER) ---------- */
static void LCD_ClearAll(void)
{
    /* 16x2 varsayýmý: iki satýrý da boþlukla temizle */
    Lcd_Goto(1,1);
    Lcd_Puts("                ");
    Lcd_Goto(2,1);
    Lcd_Puts("                ");
}

static void LCD_Update_ValuesOnly(uint16_t adcValue)
{
    char tbuf[12];
    char abuf[6];

    /* 1. satýr: saat */
    FormatTime(tbuf, sizeof(tbuf), g_h, g_m, g_s);
    Lcd_Goto(1,1);
    Lcd_Puts(tbuf);

    /* 2. satýr: 3 char metin + adc(4) saða */
    (void)snprintf(abuf, sizeof(abuf), "%4u", (unsigned)adcValue);

    Lcd_Goto(2,1);
    Lcd_Puts(g_text3);

    /* ADC'yi sað tarafa koy: kolon 13-16 (16x2 için) */
    Lcd_Goto(2,13);
    Lcd_Puts(abuf);
}

/* ---------- ADC (PE3/AIN0) ---------- */
static void ADC0_Init_PE3_AIN0(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) { }
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) { }

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    ADCSequenceDisable(ADC0_BASE, 3);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);
}

static uint16_t ReadADC12(void)
{
    uint32_t v;
    v = 0;

    ADCIntClear(ADC0_BASE, 3);
    ADCProcessorTrigger(ADC0_BASE, 3);
    while(!ADCIntStatus(ADC0_BASE, 3, false)) { }
    ADCSequenceDataGet(ADC0_BASE, 3, &v);

    return (uint16_t)(v & 0x0FFF);
}

/* ---------- PF4 ---------- */
static void GPIOF_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) { }

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
}

static uint8_t PF4_IsPressed(void)
{
    return (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) ? 1u : 0u;
}

/* ---------- PARSE ---------- */
static void ProcessLine(const char *line)
{
    unsigned hh, mm, ss;

    if (!line || line[0] == '\0') return;

    if (line[0] == '?')
    {
        hh = mm = ss = 0;
        if (sscanf(line + 1, "%u:%u:%u", &hh, &mm, &ss) == 3)
            ApplyTime((uint8_t)hh, (uint8_t)mm, (uint8_t)ss);
    }
    else if (line[0] == '*')
    {
        if (strlen(line) >= 4)
            ApplyText3(&line[1]);
    }
}

/* ---------- UART ISR (register) ---------- */
static void UART0_ISR(void)
{
    uint32_t status;
    char c;

    status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    while (UARTCharsAvail(UART0_BASE))
    {
        c = (char)UARTCharGetNonBlocking(UART0_BASE);

        if (c == '\r') continue;

        if (c == '\n')
        {
            char line[RX_LINE_MAX];
            uint32_t n, i;

            n = g_rxIdx;
            if (n > (RX_LINE_MAX - 1)) n = (RX_LINE_MAX - 1);

            for (i = 0; i < n; i++) line[i] = g_rxLine[i];
            line[n] = '\0';

            g_rxIdx = 0;
            ProcessLine(line);
        }
        else
        {
            if (g_rxIdx < (RX_LINE_MAX - 1))
            {
                g_rxLine[g_rxIdx] = c;
                g_rxIdx++;
            }
            else
            {
                g_rxIdx = 0;
            }
        }
    }
}

/* ---------- TIMER ISR (register) ---------- */
static void Timer0A_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    g_s++;
    if (g_s >= 60) { g_s = 0; g_m++; }
    if (g_m >= 60) { g_m = 0; g_h++; }
    if (g_h >= 24) { g_h = 0; }

    g_sendFlag = true;
    g_lcdFlag  = true;

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
                 (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) ^ GPIO_PIN_1));
}

/* ---------- UART init + register ---------- */
static void UART0_Init_WithRegister(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) { }
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) { }

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUDRATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTEnable(UART0_BASE);

    UARTIntRegister(UART0_BASE, UART0_ISR);
    UARTIntDisable(UART0_BASE, 0xFFFFFFFF);
    UARTIntClear(UART0_BASE, 0xFFFFFFFF);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    IntEnable(INT_UART0);
}

/* ---------- Timer init + register ---------- */
static void Timer0A_Init_1Hz_WithRegister(void)
{
    uint32_t clk;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) { }

    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);

    clk = SysCtlClockGet();
    TimerLoadSet(TIMER0_BASE, TIMER_A, clk - 1);

    TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0A_ISR);
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);

    TimerEnable(TIMER0_BASE, TIMER_A);
}

/* ---------- main ---------- */
int main(void)
{
    uint16_t adc;

    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    IntMasterDisable();

    Lcd_init();
    LCD_ClearAll();

    GPIOF_Init();
    ADC0_Init_PE3_AIN0();

    UART0_Init_WithRegister();
    Timer0A_Init_1Hz_WithRegister();

    IntMasterEnable();

    adc = ReadADC12();
    LCD_Update_ValuesOnly(adc);

    while (1)
    {
        if (g_sendFlag)
        {
            uint8_t pressed;
            char tbuf[12];
            char out[64];

            g_sendFlag = false;

            adc = ReadADC12();
            pressed = PF4_IsPressed();

            /* PC'ye rapor */
            FormatTime(tbuf, sizeof(tbuf), g_h, g_m, g_s);
            (void)snprintf(out, sizeof(out), "R%s,%u,%u\n", tbuf, (unsigned)adc, (unsigned)pressed);
            UART0_SendString(out);
        }

        if (g_lcdFlag)
        {
            g_lcdFlag = false;
            /* LCD güncelle: sadece deðerler */
            LCD_Update_ValuesOnly(adc);
        }
    }
}
