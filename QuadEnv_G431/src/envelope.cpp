#include "envelope.h"
#include <cmath>

Envelopes envelopes;

void Envelopes::calcEnvelopes()
{
	for (Envelope& env : envelope) {
		env.calcEnvelope();
	}
}


void Envelope::calcEnvelope()
{
	// Gate on
	if ((gatePort->IDR & (1 << gatePin)) != 0) {

		sustain = ADC_array.sustain;
		static constexpr uint32_t maxLevel = static_cast<uint32_t>(1.2f * pwmLength);		// Target amplitude of attack phase

		switch (gateState) {
		case gateStates::off:
			gateState = gateStates::attack;
			break;

		case gateStates::release:
			gateState = gateStates::attack;
			break;

		case gateStates::attack: {

			//ADC_array.attack
			currentLevel = static_cast<float>(maxLevel) - static_cast<float>(maxLevel - currentLevel) * 0.999f;

			if (currentLevel >= 4096.0f) {
				currentLevel = 4095.0f;
				gateState = gateStates::decay;
			}

			break;

		}

		case gateStates::decay: {
			decay = ADC_array.decay;

			if (currentLevel > sustain) {
				currentLevel = sustain + (currentLevel - sustain) * 0.999f;
			}

			if (currentLevel <= sustain + 1.5f) {				// add a little extra to avoid getting stuck in infinitely small decrease
				currentLevel = sustain;
				gateState = gateStates::sustain;
			}


			break;
		}
		case gateStates::sustain:
			currentLevel = sustain;
			break;
		}

	} else {
		if (currentLevel > 0.0f) {
			gateState = gateStates::release;
			release = ADC_array.release;

			if (currentLevel > 1.0f) {
				currentLevel = currentLevel * 0.999f;
			} else {
				currentLevel = 0.0f;
			}

		} else {
			gateState = gateStates::off;
		}
	}

	*outputChn = static_cast<uint32_t>(currentLevel);
}
