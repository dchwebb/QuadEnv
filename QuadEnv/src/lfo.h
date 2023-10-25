#pragma once

#include "initialisation.h"
#include "configManager.h"

struct LFOPots {
	uint16_t speed;
	uint16_t spread;
	uint16_t fadeIn;
	uint16_t level;
};

extern volatile LFOPots& adc;

struct LFO {
public:
	LFO(volatile uint32_t* outputChn, GPIO_TypeDef* gatePort, uint8_t gatePin)
	 : outputChn{outputChn}, gatePort{gatePort}, gatePin{gatePin} {}

	void calcLFO(uint32_t spread);				// Sets the DAC level for LFO

private:
	float CordicCos(uint32_t pos);
	float CordicExp(float x);

	float              currentLevel = 0.0f;		// The current level of the envelope (held as a float for accuracy of calulculation)
	float              currentSpeed = 0.0f;		// The current fade-in speed of the envelope
	uint32_t           lfoCosPos = 0;			// Position of cordic cosine wave in q1.31 format

	// Hardware settings for each envelope (DAC Output, GPIO gate input)
	volatile uint32_t* outputChn;
	GPIO_TypeDef*      gatePort;
	uint8_t            gatePin;
};


struct LFOs {

public:
	void calcLFOs();							// Calls calculation on all contained envelopes
	static void VerifyConfig();					// Check config is valid (must be static in order to store pointer)

	struct {
		float levelFadeIn = defaultLevelFadeIn;
		float speedFadeIn = defaultSpeedFadeIn;
	} settings;

	float levelFadeInScale = (1.0f - settings.levelFadeIn) / 4096.0f;
	float speedFadeInScale = (1.0f - settings.speedFadeIn) / 4096.0f;

	ConfigSaver configSaver = {
		.settingsAddress = &settings,
		.settingsSize = sizeof(settings),
		.validateSettings = &VerifyConfig
	};

private:
	static constexpr float defaultLevelFadeIn = 0.999996f;
	static constexpr float defaultSpeedFadeIn = 0.9999f;
	LFO lfo[4] = {
			{&(DAC1->DHR12R1), GPIOB, 6},		// PA4 Env1
			{&(DAC1->DHR12R2), GPIOB, 5},		// PA5 Env2
			{&(DAC3->DHR12R2), GPIOB, 4},		// PB1 Env3
			{&(DAC3->DHR12R1), GPIOB, 3} 		// PA2 Env4
	};
};

extern LFOs lfos;
