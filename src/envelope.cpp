#include "envelope.h"
#include <cmath>

Envelopes envelopes;

void Envelopes::calcEnvelopes()
{
//	DEBUG_ON

	envelope[0].calcEnvelope();

//	for (Envelope& env : envelope) {
//		env.calcEnvelope();
//	}

//	DEBUG_OFF
}


void Envelope::calcEnvelope()
{
	// Gate on
	if ((gatePort->IDR & (1 << gatePin)) == 0) {

		GPIOA->ODR |= GPIO_ODR_OD5;

		longADSR = (shortPort->IDR & (1 << shortPin)) != 0;

		sustain = 2048;		//ADC_array.sustain;

		switch (gateState) {
		case gateStates::off:
			gateState = gateStates::attack;
			break;

		case gateStates::release:
			gateState = gateStates::attack;
			break;

		case gateStates::attack: {

			//ADC_array.attack
			currentLevel = 5000.0f - static_cast<float>(5000 - currentLevel) * 0.999f;

			if (currentLevel >= 4095.0f) {
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
			GPIOA->ODR &= ~GPIO_ODR_OD5;
		}
	}

	*outputChn = static_cast<uint32_t>(currentLevel);

}
