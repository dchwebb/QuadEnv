#include "initialisation.h"
#include "envelope.h"
#include "usb.h"
#include "configManager.h"

volatile uint32_t SysTickVal;
volatile ADSR adsr;

extern "C" {
#include "interrupts.h"
}

Config config{&envelopes.configSaver};		// Initialise configuration to handle saving and restoring lfo settings

int main(void)
{
	SystemInit();							// Activates floating point coprocessor and resets clock
	SystemClock_Config();					// Configure the clock and PLL
	SystemCoreClockUpdate();				// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL
	InitSysTick();
	InitDAC();
	InitIO();
	InitOutputTimer();
	InitADC(reinterpret_cast<volatile uint16_t*>(&adsr));
	InitCordic();
	config.RestoreConfig();
	usb.InitUSB();

	while (1) {
		usb.cdc.ProcessCommand();			// Check for incoming CDC commands
	}
}

