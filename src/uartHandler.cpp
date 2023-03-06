#include "uartHandler.h"

volatile uint8_t uartCmdPos = 0;
volatile char uartCmd[100];
volatile bool uartCmdRdy = false;

// Manages communication to ST Link debugger UART1 on PA2 (TX), PA3 (RX) [PB6 (TX) and PB7 (RX) not connected]
void InitUart() {
	// MODER 00: Input mode, 01: General purpose output mode, 10: Alternate function mode, 11: Analog mode (reset state)

	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;				// Enable GPIO port
	RCC->APBENR1 |= RCC_APBENR1_USART2EN;

	GPIOA->MODER &= ~GPIO_MODER_MODE2_0;			// Set alternate function on PB6
	GPIOA->AFR[0] |= 1 << GPIO_AFRL_AFSEL2_Pos;		// Alternate function for USART2_TX is AF1
	GPIOA->MODER &= ~GPIO_MODER_MODE3_0;			// Set alternate function on PB7
	GPIOA->AFR[0] |= 1 << GPIO_AFRL_AFSEL3_Pos;		// Alternate function for USART2_RX is AF1

	// By default clock source is muxed to peripheral clock 2 which is system clock (change clock source in RCC->CCIPR1)
	// Calculations depended on oversampling mode set in CR1 OVER8. Default = 0: Oversampling by 16
	int USARTDIV = (SystemCoreClock) / 230400;		//clk / desired_baud
	USART2->BRR = USARTDIV & ~8;					// BRR[3] must not be set
	USART2->CR1 &= ~USART_CR1_M;					// 0: 1 Start bit, 8 Data bits, n Stop bit; 1: 1 Start bit, 9 Data bits, n Stop bit
	USART2->CR3 |= USART_CR3_OVRDIS;				// Disable buffer overrun interrupt
	USART2->CR1 |= USART_CR1_RE;					// Receive enable
	USART2->CR1 |= USART_CR1_TE;					// Transmitter enable

	// Set up interrupts
	USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
	NVIC_SetPriority(USART2_IRQn, 3);				// Lower is higher priority
	NVIC_EnableIRQ(USART2_IRQn);

	USART2->CR1 |= USART_CR1_UE;					// USART Enable

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
	while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
	USART2->TDR = c;
}

void uartSendString(const char* s) {
	char c = s[0];
	uint8_t i = 0;
	while (c) {
		while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART2->TDR = c;
		c = s[++i];
	}
}

void uartSendString(const std::string& s) {
	for (char c : s) {
		while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART2->TDR = c;
	}
}

extern "C" {

// USART Decoder
void USART2_IRQHandler() {
	if (!uartCmdRdy) {
		uartCmd[uartCmdPos] = USART2->RDR; 				// accessing RDR automatically resets the receive flag
		if (uartCmd[uartCmdPos] == 10) {
			uartCmdRdy = true;
			uartCmdPos = 0;
		} else {
			uartCmdPos++;
		}
	}
}
}
