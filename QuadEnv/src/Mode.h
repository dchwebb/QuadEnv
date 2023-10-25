#pragma once

#include "initialisation.h"
#include "configManager.h"

class Mode {
public:
	void CheckButton();
	static void VerifyConfig();

	ConfigSaver configSaver = {
		.settingsAddress = &settings,
		.settingsSize = sizeof(settings),
		.validateSettings = &VerifyConfig
	};

	struct {
		bool modeButton = false;
	} settings;
private:
	bool invertBtnDown = false;					// To manage debouncing
	uint32_t invertBtnUp = 0;					// Store systick time button released for debouncing

};


