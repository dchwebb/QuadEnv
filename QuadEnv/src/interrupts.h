void SysTick_Handler(void) {

	SysTickVal++;
}

void TIM4_IRQHandler() {
	TIM4->SR &= ~TIM_SR_UIF;
	mode.CheckButton();
	if (mode.settings.appMode == Mode::AppMode::envelope) {
		envelopes.calcEnvelopes();
	} else {
		lfos.calcLFOs();
	}
}

void USB_LP_IRQHandler() {
	usb.USBInterruptHandler();
}

void NMI_Handler(void) {}

void HardFault_Handler(void) {
	while (1) {}
}

void MemManage_Handler(void) {
	while (1) {}
}

void BusFault_Handler(void) {
	while (1) {}
}

void UsageFault_Handler(void) {
	while (1) {}
}

void SVC_Handler(void) {}

void DebugMon_Handler(void) {}

void PendSV_Handler(void) {}

