#include "initialisation.h"
#include "envelope.h"
#include "uartHandler.h"
#include <cmath>


volatile uint32_t SysTickVal;
volatile ADSR ADC_array;

extern "C" {
#include "interrupts.h"
}

uint32_t buttonDebounce;

extern uint32_t SystemCoreClock;
int main(void)
{
	SystemInit();							// Activates floating point coprocessor and resets clock
	SystemClock_Config();					// Configure the clock and PLL
	SystemCoreClockUpdate();				// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL
	InitSysTick();
	InitUart();
	InitIO();
	InitADC(reinterpret_cast<volatile uint16_t*>(&ADC_array));
	InitPWMTimer();

	while (1) {
		if ((GPIOC->IDR & GPIO_IDR_ID13) != 0 && SysTickVal > buttonDebounce + 1000) {
			buttonDebounce = SysTickVal;
			uartSendString("Test\r\n");
		}
	}
}

