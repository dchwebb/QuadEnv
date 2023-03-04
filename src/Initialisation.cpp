#include "Initialisation.h"


void SystemClock_Config()
{

	volatile uint32_t tmpreg [[maybe_unused]];

	// Enable the APB2 peripheral clock
	SET_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);
	tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);	// Delay after an RCC peripheral clock enabling

	SET_BIT(RCC->APBENR1, RCC_APBENR1_PWREN);				// Power interface clock enable
	tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_PWREN);		// Delay after an RCC peripheral clock enabling

	// Calibration to adjust frequency of the internal HSI RC
#define RCC_HSICALIBRATION_DEFAULT     64U					// Default HSI calibration trimming value

	MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, RCC_HSICALIBRATION_DEFAULT << RCC_ICSCR_HSITRIM_Pos);

	MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, 0);					// HSI48 clock division factor 0 = 1; default = 4

	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, 1);			// Set Flash Latency to 1 wait state
	while ((FLASH->ACR & FLASH_ACR_LATENCY) != 1);			// Wait until Flash Latency has been set

	// Leave other clock dividers at default (fastest)
}

void InitSysTick()
{
	SysTick_Config(SystemCoreClock / SYSTICK);		// gives 1ms
	NVIC_SetPriority(SysTick_IRQn, 0);
}


void InitPWMTimer()
{
	// TIM3: Channel 1: *PA6 (AF1), PB4 (AF1), PB8 (AF3), PB6 (AF12), PB7 (AF11), PC6 (AF1)
	// 		 Channel 2: *PA7 (AF1), PB3 (AF3), PB5 (AF1), PB9 (AF3), PB6 (AF13), PC7 (AF1), PC14 (AF11)
	// 		 Channel 3: *PA8 (AF11), PB0 (AF1), PB5 (AF3), PB6 (AF3), PC15 (AF3)
	// 		 Channel 4: PA8 (AF12), PB1 (AF1), *PB7 (AF3), PB6 (AF3), PC15 (AF3)
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
	RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
	RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

	// Enable channel 1, 2, 3 PWM output pins on PA6, PA7, PA8
	GPIOA->MODER &= ~(GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0 | GPIO_MODER_MODE8_0);	// 00: Input mode; 01: General purpose output mode; 10: Alternate function mode; 11: Analog mode (default)
	GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL6_0 | GPIO_AFRL_AFSEL7_0);		// AF1
	GPIOA->AFR[1] |= 11 << GPIO_AFRH_AFSEL8_Pos;					// AF11

	// Enable channel 4 PWM output pin on PB7
	GPIOB->MODER &= ~GPIO_MODER_MODE7_0;
	GPIOB->AFR[0] |= 3 << GPIO_AFRL_AFSEL7_Pos;						// AF3

	// Timing calculations: Clock = 64MHz / (PSC + 1) = 32m counts per second
	// ARR = number of counts per PWM tick = 4096
	// 32m / ARR = 7.812kHz of PWM square wave with 4096 levels of output
	TIM3->CCMR1 |= TIM_CCMR1_OC1PE;					// Output compare 1 preload enable
	TIM3->CCMR1 |= TIM_CCMR1_OC2PE;					// Output compare 2 preload enable
	TIM3->CCMR2 |= TIM_CCMR2_OC3PE;					// Output compare 3 preload enable
	TIM3->CCMR2 |= TIM_CCMR2_OC4PE;					// Output compare 4 preload enable

	TIM3->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);	// 0110: PWM mode 1 - In upcounting, channel 1 active if TIMx_CNT<TIMx_CCR1
	TIM3->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2);	// 0110: PWM mode 1 - In upcounting, channel 2 active if TIMx_CNT<TIMx_CCR2
	TIM3->CCMR2 |= (TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2);	// 0110: PWM mode 1 - In upcounting, channel 3 active if TIMx_CNT<TIMx_CCR3
	TIM3->CCMR2 |= (TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2);	// 0110: PWM mode 1 - In upcounting, channel 3 active if TIMx_CNT<TIMx_CCR3

	TIM3->CCR1 = 0x800;								// Initialise PWM level to midpoint (PWM level set in ble_hid.cpp)
	TIM3->CCR2 = 0x800;
	TIM3->CCR3 = 0x800;
	TIM3->CCR4 = 0x800;

	TIM3->ARR = 0xFFF;								// Total number of PWM ticks = 4096
	TIM3->PSC = 1;									// Should give ~7.8kHz
	TIM3->CR1 |= TIM_CR1_ARPE;						// 1: TIMx_ARR register is buffered
	TIM3->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E);		// Capture mode enabled / OC1 signal is output on the corresponding output pin
	TIM3->EGR |= TIM_EGR_UG;						// 1: Re-initialize the counter and generates an update of the registers
	TIM3->CR1 |= TIM_CR1_CEN;						// Enable counter
}


void InitADC(volatile uint16_t* ADC_array)
{
	// Initialize Clocks
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	RCC->APBENR2 |= RCC_APBENR2_ADCEN;
	//RCC->CCIPR |= RCC_CCIPR_ADCSEL_1;				// 00: System clock (Default); 0b10: HSIKER

	DMA1_Channel1->CCR &= ~DMA_CCR_EN;
	DMA1_Channel1->CCR |= DMA_CCR_CIRC;				// Circular mode to keep refilling buffer
	DMA1_Channel1->CCR |= DMA_CCR_MINC;				// Memory in increment mode
	DMA1_Channel1->CCR |= DMA_CCR_PSIZE_0;			// Peripheral size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Channel1->CCR |= DMA_CCR_MSIZE_0;			// Memory size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Channel1->CCR |= DMA_CCR_PL_0;				// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	DMA1->IFCR = 0x3F << DMA_IFCR_CGIF1_Pos;		// clear all five interrupts for this stream

	DMAMUX1_Channel0->CCR |= 5; 					// DMA request MUX input 5 = ADC (See p.201)
	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF0; // Channel 1 Clear synchronization overrun event flag

	ADC1->CR |= ADC_CR_ADVREGEN;					// Enable ADC internal voltage regulator

	// Wait until voltage regulator settled
	volatile uint32_t wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}
	while ((ADC1->CR & ADC_CR_ADVREGEN) != ADC_CR_ADVREGEN) {}

	//ADC12_COMMON->CCR |= ADC_CCR_CKMODE;			// adc_hclk/4 (Synchronous clock mode)
	ADC1->CFGR1 |= ADC_CFGR1_CONT;					// 1: Continuous conversion mode for regular conversions
	ADC1->CFGR1 |= ADC_CFGR1_OVRMOD;				// Overrun Mode 1: ADC_DR register is overwritten with the last conversion result when an overrun is detected.
	ADC1->CFGR1 |= ADC_CFGR1_DMACFG;				// 0: DMA One Shot Mode selected, 1: DMA Circular Mode selected
	ADC1->CFGR1 |= ADC_CFGR1_DMAEN;					// Enable ADC DMA

	// A0 = PA0 (ADC_IN0); A1 = PA1 (ADC_IN1); A2 = PA4 (ADC_IN4); A3 = PB1 (ADC_IN18)
	ADC1->CFGR1 &= ~ADC_CFGR1_CHSELRMOD;			// In this mode bits in ADC_CHSELR activate inputs
	ADC1->CHSELR |= ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL4 | ADC_CHSELR_CHSEL18;

	// Start calibration
	ADC1->CR |= ADC_CR_ADCAL;
	while ((ADC1->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL) {};

	// Enable ADC
	ADC1->CR |= ADC_CR_ADEN;
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0) {}

	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF0; // Channel 1 Clear synchronization overrun event flag
	DMA1->IFCR = 0x3F << DMA_IFCR_CGIF1_Pos;		// clear all five interrupts for this stream

	DMA1_Channel1->CNDTR |= ADC_BUFFER_LENGTH;		// Number of data items to transfer (ie size of ADC buffer)
	DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR));	// Configure the peripheral data register address 0x40022040
	DMA1_Channel1->CMAR = (uint32_t)(ADC_array);	// Configure the memory address (note that M1AR is used for double-buffer mode) 0x24000040

	DMA1_Channel1->CCR |= DMA_CCR_EN;				// Enable DMA and wait
	wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}

	ADC1->CR |= ADC_CR_ADSTART;						// Start ADC
}

