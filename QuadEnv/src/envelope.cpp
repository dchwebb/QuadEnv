#include "envelope.h"
#include "mode.h"
#include <cmath>
#include <cstring>

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

		sustain = adsr.sustain;

		switch (gateState) {
		case gateStates::off:
			gateState = gateStates::attack;
			break;

		case gateStates::release:
			gateState = gateStates::attack;
			break;

		case gateStates::attack: {

			attack = std::round(((attack * 31.0f) + static_cast<float>(adsr.attack)) / 32.0f);

			// fullRange = value of fully charged capacitor; capacitor value is 4096 where cap is charged enough to trigger decay phase
			const float fullRange = 5000.0f;

			// scales attack pot to allow more range at low end of pot, exponentially longer times at upper end
			const float maxDurationMult = envelopes.settings.durationMult * 0.9f * 0.578;			// 0.578 allows duration to be set in seconds

			// RC value - attackScale represents R component; maxDurationMult represents capacitor size (Reduce rc for a steeper curve)
			const float tempAttack = attack / 4096.f;				// Space constraints mean we can't use pow function on floats
			const float rc = tempAttack * tempAttack * tempAttack * maxDurationMult;		// Using a^3 for fast approximation for measured charging rate (^2.9)

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

				currentLevel = fullRange - (fullRange - currentLevel) * CordicExp(-timeStep / rc);

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
			// scales decay pot to allow more range at low end of pot, exponentially longer times at upper end
			const float maxDurationMult = envelopes.settings.durationMult * 5.28f * 0.227f;		// to scale maximum delay time

			// RC value - decayScale represents R component; maxDurationMult represents capacitor size
			const float rc = std::pow(static_cast<float>(adsr.decay) / 4096.0f, 2.0f) * maxDurationMult;		// Use x^2 as approximation for measured x^2.4

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

				currentLevel = sustain + (currentLevel - sustain) * CordicExp(-timeStep / rc);

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

			const float maxDurationMult = envelopes.settings.durationMult * 1.15f;		// to scale maximum delay time

			// RC value - decayScale represents R component; maxDurationMult represents capacitor size
			const float rc = std::pow(static_cast<float>(adsr.release) / 4096.0f, 2.0f) * maxDurationMult;
			if (rc != 0.0f && currentLevel > 1.0f) {
				/*
				 * Long hand calculations:
				 * float xPos = -rc * std::log(currentLevel / 4096.0f);
				 * float newXPos = xPos + timeStep;
				 * float newYPos = std::exp(-newXPos / rc);
				 * currentLevel = newYPos * 4096.0f;
				 */

				currentLevel = currentLevel * CordicExp(-timeStep / rc);
			} else {
				currentLevel = 0.0f;
			}

		} else {
			gateState = gateStates::off;
		}
	}

	if (mode.settings.modeButton) {					// Invert envelope when mode button activated
		*outputChn = 4095 - static_cast<uint32_t>(currentLevel);
		*ledPWM = 4095 - static_cast<uint32_t>(currentLevel);
	} else {
		*outputChn = static_cast<uint32_t>(currentLevel);
		*ledPWM = static_cast<uint32_t>(currentLevel);
	}
}


float Envelope::CordicExp(float x)
{
	// use CORDIC sinh function and generate e^x = sinh(x) + cosh(x)
	CORDIC->CSR = (6 << CORDIC_CSR_FUNC_Pos) | 		// 0: Cos, 1: Sin, 2: Phase, 3: Modulus, 4: Arctan, 5: cosh, 6: sinh, 7: Arctanh, 8: ln, 9: Square Root
			CORDIC_CSR_SCALE_0 |					// Must be 1 for sinh
			CORDIC_CSR_NRES |						// 2 Results as we need both sinh and cosh
			(6 << CORDIC_CSR_PRECISION_Pos);		// Set precision to 6 (gives 6 * 4 = 24 iterations in 6 clock cycles)

	// convert float to q1_31 format scaling x by 1/2 at the same time
	int q31;
	if (x < -1.118f) {
		q31 = (int)((x + 1.0f) * 1073741824.0f);	// as range of x is limited to -1.118 to +1.118 reduce exponent by e^-1 (note that only values from around -1.75 to 0 used in this mechanism)
	} else {
		q31 = (int)(x * 1073741824.0f);
	}

	//volatile float etest = std::exp(x);

	CORDIC->WDATA = q31;

	// convert values back to floats scaling by * 2 at the same time
	float sinh = (float)((int)CORDIC->RDATA) / 1073741824.0f;	// command will block until RDATA is ready - no need to poll RRDY flag
	float cosh = (float)((int)CORDIC->RDATA) / 1073741824.0f;
	float res = sinh + cosh;
	if (x < -1.118f) {
		return res * 0.3678794411714f;				// multiply by e^-1 to correct range offset
	} else {
		return res;
	}
}


void Envelopes::VerifyConfig()
{
	// Verify settings and update as required
	if (envelopes.settings.durationMult < 0.1f || envelopes.settings.durationMult > 9.9f) {
		envelopes.settings.durationMult = 1.0f;
	}
}
