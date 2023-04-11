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

		switch (gateState) {
		case gateStates::off:
			gateState = gateStates::attack;
			break;

		case gateStates::release:
			gateState = gateStates::attack;
			break;

		case gateStates::attack: {

			//attack = std::round(((attack * 31.0f) + static_cast<float>(ADC_array.attack)) / 32.0f);

			// fullRange = value of fully charged capacitor; comparitor value is 4096 where cap is charged enough to trigger decay phase
			const float fullRange = 5000.0f;

			// scales attack pot to allow more range at low end of pot, exponentially longer times at upper end
			const float maxDurationMult = 0.9f / 1.73f;		// 1.73 allows duration to be set in seconds

			// RC value - attackScale represents R component; maxDurationMult represents capacitor size (Reduce rc for a steeper curve)
			float rc = 0.80f + sqrt(ADC_array.attack / 4096.f) * 0.20f;		// Using a^3 for fast approximation for measured charging rate (^2.9)

			currentLevel = fullRange - (fullRange - currentLevel) * rc;

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
		}
	}

	*outputChn = static_cast<uint32_t>(currentLevel);
}
