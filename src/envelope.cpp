#include "envelope.h"
#include <cmath>


void Envelopes::calcEnvelopes()
{
//	DEBUG_ON

	for (Envelope& env : envelope) {
		env.calcEnvelope();
	}

//	DEBUG_OFF
}


void Envelope::calcEnvelope()
{
	// Gate on
	if ((gatePort->IDR & (1 << gatePin)) == 0) {

		longADSR = (shortPort->IDR & (1 << shortPin)) != 0;

		sustain = ADC_array.sustain;

		switch (gateState) {
		case gateStates::off:
			gateState = gateStates::attack;
			break;

		case gateStates::release:
			gateState = gateStates::attack;
			break;

		case gateStates::attack: {

			attack = std::round(((attack * 31.0f) + static_cast<float>(ADC_array.attack)) / 32.0f);

			// fullRange = value of fully charged capacitor; comparitor value is 4096 where cap is charged enough to trigger decay phase
			const float fullRange = 5000.0f;

			// scales attack pot to allow more range at low end of pot, exponentially longer times at upper end
			float maxDurationMult = (longADSR ? 7.7f : 0.9f) / 1.73f;		// 1.73 allows duration to be set in seconds

			// RC value - attackScale represents R component; maxDurationMult represents capacitor size (Reduce rc for a steeper curve)
			float rc = std::pow(static_cast<float>(attack) / 4096.f, 3.0f) * maxDurationMult;		// Using a^3 for fast approximation for measured charging rate (^2.9)

			if (rc != 0.0f) {
				/*
				 * Long hand calculations:
				 * Capacitor charging equation: Vc = Vs(1 - e ^ -t/RC)
				 * 1. Invert capacitor equation to calculate current 'time' based on y/voltage value
				 * float ln = std::log(1.0f - (currentLevel / fullRange));
				 * float xPos = -rc * ln;
				 * float newXPos = xPos + timeStep;		// Add timeStep (based on sample rate) to current X position
				 *
				 * 2. Calculate exponential of time for capacitor charging equation
				 * float exponent = -newXPos / rc;
				 * float newYPos = 1.0f - std::exp(exponent);
				 * currentLevel = newYPos * fullRange;
				 */

				//currentLevel = fullRange - (fullRange - currentLevel) * CordicExp(-timeStep / rc);

			} else {
				currentLevel = fullRange;
			}

			if (currentLevel >= 4095.0f) {
				currentLevel = 4095.0f;
				gateState = gateStates::decay;
			}

			break;

		}

		case gateStates::decay: {
			decay = ADC_array.decay;

			// scales decay pot to allow more range at low end of pot, exponentially longer times at upper end
			float maxDurationMult = (longADSR ? 44.0f : 5.28f) / 4.4;		// to scale maximum delay time

			// RC value - decayScale represents R component; maxDurationMult represents capacitor size
			float rc = std::pow((float)decay / 4096.0f, 2.0f) * maxDurationMult;		// Use x^2 as approximation for measured x^2.4

			if (rc != 0.0f && currentLevel > sustain) {
				/*
				 * Long hand calculations:
				 * Capacitor discharge equation: Vc = Vo * e ^ -t/RC
				 * 1. Invert capacitor discharge equation to calculate current 'time' based on y/voltage
				 * float yHeight = 4096.0f - sustain;		// Height of decay curve
				 * float xPos = -rc * std::log((currentLevel - sustain) / yHeight);
				 * float newXPos = xPos + timeStep;
				 *
				 * 2. Calculate exponential of time for capacitor discharging equation
				 * float exponent = -newXPos / rc;
				 * float newYPos = std::exp(exponent);		// Capacitor discharging equation
				 * currentLevel = (newYPos * yHeight) + sustain;
				 */

				//currentLevel = sustain + (currentLevel - sustain) * CordicExp(-timeStep / rc);

			} else {
				currentLevel = 0.0f;
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

			float maxDurationMult = (longADSR ? 14.585f : 1.15f);		// to scale maximum delay time

			// RC value - decayScale represents R component; maxDurationMult represents capacitor size
			float rc = std::pow(static_cast<float>(release) / 4096.0f, 2.0f) * maxDurationMult;
			if (rc != 0.0f && currentLevel > 1.0f) {
				/*
				 * Long hand calculations:
				 * float xPos = -rc * std::log(currentLevel / 4096.0f);
				 * float newXPos = xPos + timeStep;
				 * float newYPos = std::exp(-newXPos / rc);
				 * currentLevel = newYPos * 4096.0f;
				 */

				//currentLevel = currentLevel * CordicExp(-timeStep / rc);
			} else {
				currentLevel = 0.0f;
			}

		} else {
			gateState = gateStates::off;
		}
	}

	*outputChn = static_cast<uint32_t>(currentLevel);

}
