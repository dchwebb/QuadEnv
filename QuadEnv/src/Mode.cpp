#include <Mode.h>

Mode mode;

void Mode::CheckButton()
{
	if ((GPIOF->IDR & GPIO_IDR_ID0) == 0) {
		if (!modeBtnOn) {
			modeBtnDown = SysTickVal;
			modeFlash = 0;
		} else if (SysTickVal > modeBtnDown + longPress) {		// Once a long press has been registered flash the mode button rapidly
			if (++modeFlash > flashCount) {
				modeFlash = 0;
				if ((GPIOC->ODR & GPIO_ODR_OD13) == 0) {
					GPIOC->ODR |= GPIO_ODR_OD13;
				} else {
					GPIOC->ODR &= ~GPIO_ODR_OD13;
				}
			}
		}
		modeBtnOn = true;

	} else if (modeBtnOn) {
		// Check button has been down long enough not to count as a bounce
		if (SysTickVal > modeBtnDown + 100) {
			modeBtnUp = SysTickVal;

			// Check if long or short click
			if (SysTickVal > modeBtnDown + longPress) {
				settings.appMode = settings.appMode == AppMode::envelope ? AppMode::lfo : AppMode::envelope;
			} else {
				settings.modeButton = !settings.modeButton;
			}
			SetMode();
			config.SaveConfig();
		}
		modeBtnOn = false;
	}
}

void Mode::SetMode()
{
	// Verify settings and update as required
	if (mode.settings.modeButton) {
		GPIOC->ODR |= GPIO_ODR_OD13;
	} else {
		GPIOC->ODR &= ~GPIO_ODR_OD13;
	}
}
