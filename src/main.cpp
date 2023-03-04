#include "initialisation.h"
//#include "envelope.h"
//#include "uartHandler.h"
#include <cmath>


volatile uint32_t SysTickVal;
volatile uint16_t ADC_array[ADC_BUFFER_LENGTH];

uint32_t buttonDebounce;
//Envelopes envelopes;


extern "C" {
#include "interrupts.h"
}


extern uint32_t SystemCoreClock;
int main(void)
{
	SystemInit();							// Activates floating point coprocessor and resets clock
	SystemClock_Config();					// Configure the clock and PLL
	SystemCoreClockUpdate();				// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL
	InitSysTick();

//	InitIO();
//	InitEnvTimer();
	InitADC(reinterpret_cast<volatile uint16_t*>(&ADC_array));
//	InitUart();
//	InitCordic();


	while (1) {
//		if ((GPIOC->IDR & GPIO_IDR_ID13) != 0 && SysTickVal > buttonDebounce + 1000) {
//			buttonDebounce = SysTickVal;
//		}
	}
}

