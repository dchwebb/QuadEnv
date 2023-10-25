#pragma once

#include "initialisation.h"
#include "configManager.h"

class Mode {
public:
	enum class AppMode {envelope, lfo};
	void CheckButton();
	static void SetMode();

	ConfigSaver configSaver = {
		.settingsAddress = &settings,
		.settingsSize = sizeof(settings),
		.validateSettings = &SetMode
	};

	struct {
		bool modeButton = false;
		AppMode appMode = AppMode::envelope;
	} settings;
private:
	static constexpr uint32_t longPress = 1500;
	static constexpr uint32_t flashCount = 5000;
	bool modeBtnOn = false;					// To manage debouncing
	uint32_t modeBtnUp = 0;					// Store systick time button released for debouncing
	uint32_t modeBtnDown = 0;
	uint32_t modeFlash = 0;					// Used to time flashing to show application mode has been switched
};

extern Mode mode;
