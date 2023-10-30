#pragma once

#include "initialisation.h"
#include "configManager.h"

struct ADSR {
	uint16_t attack;
	uint16_t decay;
	uint16_t sustain;
	uint16_t release;
};
extern volatile ADSR adsr;


struct Envelope {
public:
	Envelope(volatile uint32_t* outputChn, GPIO_TypeDef* gatePort, uint8_t gatePin, volatile uint32_t* ledPWM)
	 : outputChn{outputChn}, gatePort{gatePort}, gatePin{gatePin}, ledPWM{ledPWM} {}

	void calcEnvelope();							// Sets the DAC level for envelope

private:
	float CordicExp(float x);

	const float     timeStep = 1.0f / SAMPLERATE;	// one time unit - corresponding to sample time

	float           attack = 800.0f;				// Store the ADSR values based on the pot values (mainly for debugging)
	float           sustain = 4095.0f;
	float           currentLevel = 0.0f;			// The current level of the envelope (held as a float for accuracy of calulculation)

	enum class      gateStates {off, attack, decay, sustain, release};
	gateStates      gateState = gateStates::off;

	// Hardware settings for each envelope (DAC Output, GPIO gate input, LED PWM)
	volatile uint32_t* outputChn;
	GPIO_TypeDef*      gatePort;
	uint8_t            gatePin;
	volatile uint32_t* ledPWM;
};


struct Envelopes {

public:
	void calcEnvelopes();						// Calls calculation on all contained envelopes
	static void VerifyConfig();

	ConfigSaver configSaver = {
		.settingsAddress = &settings,
		.settingsSize = sizeof(settings),
		.validateSettings = &VerifyConfig
	};

	struct {
		float durationMult = 1.0f;
	} settings;

private:
	Envelope envelope[4] = {
			{&(DAC1->DHR12R1), GPIOB, 6, &TIM3->CCR1},		// Env1: PB3 Gate; PA6  PWM LED
			{&(DAC1->DHR12R2), GPIOB, 5, &TIM3->CCR2},		// Env2: PB5 Gate; PA7  PWM LED
			{&(DAC3->DHR12R2), GPIOB, 4, &TIM2->CCR3},		// Env3: PB4 Gate; PA9  PWM LED
			{&(DAC3->DHR12R1), GPIOB, 3, &TIM2->CCR4} 		// Env4: PB6 Gate; PB11 PWM LED
	};
};


extern Envelopes envelopes;
