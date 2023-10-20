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
	Envelope(volatile uint32_t* outputChn, GPIO_TypeDef* gatePort, uint8_t gatePin)
	 : outputChn{outputChn}, gatePort{gatePort}, gatePin{gatePin} {}

	void calcEnvelope();							// Sets the DAC level for envelope

private:
	float CordicExp(float x);

	const float     timeStep = 1.0f / SAMPLERATE;	// one time unit - corresponding to sample time

	bool            longADSR = false;				// True if using long ADSR settings
	float           attack = 800.0f;				// Store the ADSR values based on the pot values (mainly for debugging)
	float           sustain = 4095.0f;
	float           currentLevel = 0.0f;			// The current level of the envelope (held as a float for accuracy of calulculation)

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
	static void VerifyConfig();

	ConfigSaver configSaver = {
		.settingsAddress = &settings,
		.settingsSize = sizeof(settings),
		.validateSettings = &VerifyConfig
	};

	struct {
		float durationMult = 1.0f;
		bool invert = false;
	} settings;

private:
	Envelope envelope[4] = {
			{&(DAC1->DHR12R1), GPIOB, 6},		// PA4 Env1
			{&(DAC1->DHR12R2), GPIOB, 5},		// PA5 Env2
			{&(DAC3->DHR12R2), GPIOB, 4},		// PB1 Env3
			{&(DAC3->DHR12R1), GPIOB, 3} 		// PA2 Env4
	};

};

extern Envelopes envelopes;
