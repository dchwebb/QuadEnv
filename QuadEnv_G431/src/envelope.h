#pragma once

#include "initialisation.h"


struct ADSR {
	uint16_t attack;
	uint16_t decay;
	uint16_t sustain;
	uint16_t release;
};



extern volatile ADSR ADC_array;

struct Envelope {
public:
	Envelope(volatile uint32_t* outputChn, GPIO_TypeDef* gatePort, uint8_t gatePin)
	 : outputChn{outputChn}, gatePort{gatePort}, gatePin{gatePin} {}

	void calcEnvelope();						// Sets the DAC level for envelope

private:
	const float     timeStep = 1.0f / SAMPLERATE;	// one time unit - corresponding to sample time

	bool            longADSR = true;			// True if using long ADSR settings
	float           attack = 800.0f;			// Store the ADSR values based on the pot values (mainly for debugging)
	uint16_t        decay = 0;
	float           sustain = 4095.0f;
	uint16_t        release = 300;
	float           currentLevel = 0.0f;		// The current level of the envelope (held as a float for accuracy of calulculation)

	enum class      gateStates {off, attack, decay, sustain, release};
	gateStates      gateState = gateStates::off;

	// Hardware settings for each envelope (DAC Output, GPIO gate input)
	volatile uint32_t* outputChn;
	GPIO_TypeDef*      gatePort;
	uint8_t            gatePin;

};


struct Envelopes {

public:
	void calcEnvelopes();					// Calls calculation on all contained envelopes

private:
	bool     clockValid;					// True if a clock pulse has been received within a second
	uint32_t clockInterval;					// Clock interval in sample time
	uint32_t clockCounter;					// Counter used to calculate clock times in sample time
	uint32_t lastClock;						// Time last clock signal received in sample time
	bool     clockHigh;						// Record clock high state to detect clock transitions

	Envelope envelope[4] = {
			{&(DAC1->DHR12R1), GPIOC, 13},		// PA4 Env1
			{&(DAC1->DHR12R2), GPIOB, 14},		// PA5 Env2
			{&(DAC3->DHR12R2), GPIOB, 15},		// PB1 Env3
			{&(DAC3->DHR12R1), GPIOC,  6} 		// PA2 Env4
	};


};

extern Envelopes envelopes;
