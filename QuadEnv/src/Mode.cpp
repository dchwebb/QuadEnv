#include <Mode.h>

Mode mode;

void Mode::CheckButton()
{
	// check if mode button has been pressed with debounce
	if ((GPIOF->IDR & GPIO_IDR_ID0) == 0) {
		if (SysTickVal > invertBtnUp + 100 && !invertBtnDown) {
			invertBtnDown = true;
			settings.modeButton = !settings.modeButton;
			if (settings.modeButton) {
				GPIOC->ODR |= GPIO_ODR_OD13;
			} else {
				GPIOC->ODR &= ~GPIO_ODR_OD13;
			}
			config.SaveConfig();
		}
	} else if (invertBtnDown) {
		invertBtnUp = SysTickVal;
		invertBtnDown = false;
	}
}

void Mode::VerifyConfig()
{
	// Verify settings and update as required
	if (mode.settings.modeButton) {
		GPIOC->ODR |= GPIO_ODR_OD13;
	} else {
		GPIOC->ODR &= ~GPIO_ODR_OD13;
	}
}
