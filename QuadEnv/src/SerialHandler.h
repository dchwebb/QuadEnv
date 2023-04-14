#pragma once

#include "initialisation.h"
#include "USB.h"
#include <string>

class SerialHandler {
public:
	SerialHandler(USBHandler& usb);
	bool Command();
	void Handler(uint8_t* data, uint32_t length);

private:
	int32_t ParseInt(const std::string cmd, const char precedingChar, int low, int high);
	float ParseFloat(const std::string cmd, const char precedingChar, float low, float high);

	uint8_t cmdPending = false;
	std::string cmd;
	USBHandler* usb;
};

extern SerialHandler serial;
