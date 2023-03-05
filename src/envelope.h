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
	Envelope(volatile uint32_t* outputChn, GPIO_TypeDef* gatePort, uint8_t gatePin, GPIO_TypeDef* shortPort, uint8_t shortPin)
	 : outputChn{outputChn}, gatePort{gatePort}, gatePin{gatePin}, shortPort{shortPort}, shortPin{shortPin} {}

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

	// Hardware settings for each envelope (which ADSR bank, GPIO gate input and switch positions)
	volatile uint32_t* outputChn;
	GPIO_TypeDef*      gatePort;
	uint8_t            gatePin;
	GPIO_TypeDef*      shortPort;
	uint8_t            shortPin;
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
			{&(TIM3->CCR3), GPIOC, 13, GPIOB,  5},		// PA6 Env1
			{&(TIM3->CCR2), GPIOB, 14, GPIOB,  3},		// PA7 Env2
			{&(TIM3->CCR1), GPIOB, 15, GPIOC, 10},		// PA8 Env3
			{&(TIM3->CCR4), GPIOC,  6, GPIOB, 12} 		// PB7 Env4
	};
};

extern Envelopes envelopes;
