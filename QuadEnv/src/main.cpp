#include "initialisation.h"
#include "envelope.h"
#include "lfo.h"
#include "usb.h"
#include "configManager.h"
#include "Mode.h"

volatile uint32_t SysTickVal;
volatile ADSR adsr;
volatile LFOPots& adc = (volatile LFOPots&)adsr;

extern "C" {
#include "interrupts.h"
}

// Initialise configuration to handle saving and restoring mode, envelope and lfo settings
Config config{&mode.configSaver, &envelopes.configSaver, &lfos.configSaver};

int main(void)
{
	SystemInit();							// Activates floating point coprocessor and resets clock
	InitClocks();							// Configure the clock and PLL
	InitSysTick();
	InitDAC();
	InitIO();
	InitOutputTimer();
	InitPWMTimer();
	InitADC(reinterpret_cast<volatile uint16_t*>(&adsr));
	InitCordic();
	config.RestoreConfig();
	usb.InitUSB();

	while (1) {
		usb.cdc.ProcessCommand();			// Check for incoming CDC commands
	}
}

