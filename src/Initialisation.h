#pragma once

#include "stm32c0xx.h"

extern volatile uint32_t SysTickVal;

#define SYSTICK 1000						// 1ms
#define SAMPLERATE 48000.0f
#define ADC_BUFFER_LENGTH 4

static constexpr uint32_t pwmLength = 512;

//#define DEBUG_ON  GPIOB->ODR |= GPIO_ODR_OD9;
//#define DEBUG_OFF GPIOB->ODR &= ~GPIO_ODR_OD9;

void SystemClock_Config();
void InitSysTick();
void InitIO();
void InitEnvTimer();
void InitPWMTimer();
void InitADC(volatile uint16_t* ADC_array);
void InitUart();
