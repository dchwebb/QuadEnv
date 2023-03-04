#include "Initialisation.h"
#include "core_cm0plus.h"
#include "stm32c031xx.h"
#include "stm32c0xx.h"

void SystemClock_Config()
{
	SysTick_Config(48000);		// core_cm0plus.h

	volatile uint32_t tmpreg;

	// Enable the APB2 peripheral clock
	SET_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);
	tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);	// Delay after an RCC peripheral clock enabling

	SET_BIT(RCC->APBENR1, RCC_APBENR1_PWREN);
	tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_PWREN);		// Delay after an RCC peripheral clock enabling

	// Calibration to adjust frequency of the internal HSI RC
#define RCC_HSICALIBRATION_DEFAULT     64U					// Default HSI calibration trimming value

	MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, RCC_HSICALIBRATION_DEFAULT << RCC_ICSCR_HSITRIM_Pos);

	MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, 0);					// HSI48 clock division factor 0 = 1; default = 4

	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, 1);			// Set Flash Latency to 1 wait state
	while ((FLASH->ACR & FLASH_ACR_LATENCY) != 1);			// Wait until Flash Latency has been set

	// Leave other clock dividers at default (fastest)
}
