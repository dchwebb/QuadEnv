#include "uartHandler.h"

volatile uint8_t uartCmdPos = 0;
volatile char uartCmd[100];
volatile bool uartCmdRdy = false;

// Manages communication to ST Link debugger UART1 on PB6 (TX) and PB7 (RX)
void InitUart() {
	// MODER 00: Input mode, 01: General purpose output mode, 10: Alternate function mode, 11: Analog mode (reset state)

	RCC->IOPENR |= RCC_IOPENR_GPIOBEN;			// reset and clock control - advanced high performance bus - GPIO port B
	RCC->APBENR2 |= RCC_APBENR2_USART1EN;

	GPIOB->MODER &= ~GPIO_MODER_MODE6_0;			// Set alternate function on PB6
	GPIOB->AFR[0] |= 0 << GPIO_AFRL_AFSEL6_Pos;		// Alternate function for USART1_TX is AF0
	GPIOB->MODER &= ~GPIO_MODER_MODE7_0;			// Set alternate function on PB7
	GPIOB->AFR[0] |= 0 << GPIO_AFRL_AFSEL7_Pos;		// Alternate function for USART1_RX is AF0

	// By default clock source is muxed to peripheral clock 2 which is system clock (change clock source in RCC->CCIPR1)
	// Calculations depended on oversampling mode set in CR1 OVER8. Default = 0: Oversampling by 16
	int USARTDIV = (SystemCoreClock) / 230400;		//clk / desired_baud
	USART1->BRR = USARTDIV & ~8;					// BRR[3] must not be set
	USART1->CR1 &= ~USART_CR1_M;					// 0: 1 Start bit, 8 Data bits, n Stop bit; 1: 1 Start bit, 9 Data bits, n Stop bit
	USART1->CR3 |= USART_CR3_OVRDIS;				// Disable buffer overrun interrupt
	USART1->CR1 |= USART_CR1_RE;					// Receive enable
	USART1->CR1 |= USART_CR1_TE;					// Transmitter enable

	// Set up interrupts
	USART1->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
	NVIC_SetPriority(USART1_IRQn, 3);				// Lower is higher priority
	NVIC_EnableIRQ(USART1_IRQn);

	USART1->CR1 |= USART_CR1_UE;					// USART Enable

}


std::string IntToString(const int32_t& v) {
	return std::to_string(v);
}

std::string HexToString(const uint32_t& v, const bool& spaces) {
	char buf[20];
	if (spaces) {
		if (v != 0) {
			sprintf(buf, "%02luX %02luX %02luX %02luX", v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF);
		} else {
			sprintf(buf, " ");
		}
	} else {
		sprintf(buf, "%luX", v);
	}
	return std::string(buf);

}

std::string HexByte(const uint16_t& v) {
	char buf[50];
	sprintf(buf, "%X", v);
	return std::string(buf);

}

void uartSendChar(char c) {
	while ((USART1->ISR & USART_ISR_TXE_TXFNF) == 0);
	USART1->TDR = c;
}

void uartSendString(const char* s) {
	char c = s[0];
	uint8_t i = 0;
	while (c) {
		while ((USART1->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART1->TDR = c;
		c = s[++i];
	}
}

void uartSendString(const std::string& s) {
	for (char c : s) {
		while ((USART1->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART1->TDR = c;
	}
}

extern "C" {

// USART Decoder
void USART1_IRQHandler() {
	if (!uartCmdRdy) {
		uartCmd[uartCmdPos] = USART1->RDR; 				// accessing RDR automatically resets the receive flag
		if (uartCmd[uartCmdPos] == 10) {
			uartCmdRdy = true;
			uartCmdPos = 0;
		} else {
			uartCmdPos++;
		}
	}
}
}
