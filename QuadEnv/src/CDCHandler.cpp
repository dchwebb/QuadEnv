#include "USB.h"
#include "configManager.h"
#include "envelope.h"
#include "lfo.h"
#include "mode.h"
#include <charconv>
#include <stdarg.h>
#include <cmath>

static inline std::tuple<uint32_t, uint32_t> SplitFloat(float val)
{
	uint32_t integerPart = (uint32_t)val;
	uint32_t fractionPart = std::round((val - integerPart) * 1000000);
	return std::make_tuple(integerPart, fractionPart);
}


void CDCHandler::ProcessCommand()
{
	// Check if a command has been received from USB, parse and action as required
	if (!cmdPending) {
		return;
	}

	const std::string_view cmd {comCmd};

	if (cmd.compare("info") == 0) {		// Print diagnostic information
		// Use manual float conversion as printf with float support uses more space than we have
		auto [intDuration, frcDuration] = SplitFloat(envelopes.settings.durationMult);
		auto [intLevelFadeIn, frcLevelFadeIn] = SplitFloat(lfos.settings.levelFadeIn);

		printf("Mountjoy QuadEnv/LFO v1.0 - Current Settings:\r\n\r\n"
				"Mode: %s\r\n"
				"Duration Multiplier: %ld.%ld\r\n"
				"Fade-in rate: %ld.%ld\r\n"
				"\r\n",
				mode.settings.appMode == Mode::AppMode::envelope ? "Envelope" : "LFO",
				intDuration, frcDuration,
				intLevelFadeIn, frcLevelFadeIn);


	} else if (cmd.compare("help") == 0) {

		usb->SendString("Mountjoy QuadEnv/LFO\r\n"
				"\r\nSupported commands:\r\n"
				"info        -  Show diagnostic information\r\n"
				"mult:xx.x   -  Set envelope duration multiplier (eg 1.0 for short, 8.5 for long)\r\n"
				"fadein:xx.x -  Set lfo fade in rate (default 0.999996)\r\n"
				"savecfg     -  Save config\r\n"
				"erasecfg    -  Erase config\r\n"
				"\r\n"
#if (USB_DEBUG)
				"usbdebug    -  Start USB debugging\r\n"
				"\r\n"
#endif
		);

#if (USB_DEBUG)
	} else if (cmd.compare("usbdebug\n") == 0) {				// Configure gate LED
		USBDebug = true;
		usb->SendString("Press link button to dump output\r\n");
#endif

	} else if (cmd.compare(0, 5, "mult:") == 0) {					// Set envelope duration multiplier
		const float mult = ParseFloat(cmd, ':', 0.1f, 99.9f);
		if (mult >= 0.0f) {
			envelopes.settings.durationMult = mult;
			envelopes.VerifyConfig();
			config.SaveConfig();
		}

	} else if (cmd.compare(0, 7, "fadein:") == 0) {					// Set lfo fade in rate
		const float val = ParseFloat(cmd, ':', 0.1f, 99.9f);
		if (val >= 0.0f) {
			lfos.settings.levelFadeIn = val;
			lfos.VerifyConfig();
			config.SaveConfig();
		}

	} else if (cmd.compare("savecfg") == 0) {						// Save configuration
		config.SaveConfig();

	} else if (cmd.compare("erasecfg") == 0) {						// Erase config settings
		config.EraseConfig();

	} else {
		usb->SendString("Unrecognised command: " + std::string(cmd) + " Type 'help' for supported commands\r\n");
	}

	cmdPending = false;
}


void CDCHandler::PrintString(const char* format, ...)
{
	va_list args;
	va_start (args, format);
	vsnprintf (buf, bufSize, format, args);
	va_end (args);

	usb->SendString(buf);
}


char* CDCHandler::HexToString(const uint8_t* v, uint32_t len, const bool spaces) {
	const uint8_t byteLen = spaces ? 3 : 2;
	uint32_t pos = 0;
	len = std::min(maxStrLen / byteLen, len);

	for (uint8_t i = 0; i < len; ++i) {
		pos += sprintf(&stringBuf[pos], spaces ? "%02X " : "%02X", v[i]);
	}

	return (char*)&stringBuf;
}


char* CDCHandler::HexToString(const uint16_t v) {
	sprintf(stringBuf, "%04X", v);
	return (char*)&stringBuf;
}


void CDCHandler::DataIn()
{

}


// As this is called from an interrupt assign the command to a variable so it can be handled in the main loop
void CDCHandler::DataOut()
{
	// Check if sufficient space in command buffer
	const uint32_t newCharCnt = std::min(outBuffCount, maxCmdLen - 1 - buffPos);

	strncpy(&comCmd[buffPos], (char*)outBuff, newCharCnt);
	buffPos += newCharCnt;

	// Check if cr has been sent yet
	if (comCmd[buffPos - 1] == 13 || comCmd[buffPos - 1] == 10 || buffPos == maxCmdLen - 1) {
		comCmd[buffPos - 1] = '\0';
		cmdPending = true;
		buffPos = 0;
	}
}


void CDCHandler::ActivateEP()
{
	EndPointActivate(USBMain::CDC_In,   Direction::in,  EndPointType::Bulk);			// Activate CDC in endpoint
	EndPointActivate(USBMain::CDC_Out,  Direction::out, EndPointType::Bulk);			// Activate CDC out endpoint
	EndPointActivate(USBMain::CDC_Cmd,  Direction::in,  EndPointType::Interrupt);		// Activate Command IN EP

	EndPointTransfer(Direction::out, USBMain::CDC_Out, USBMain::ep_maxPacket);
}


void CDCHandler::ClassSetup(usbRequest& req)
{
	if (req.RequestType == DtoH_Class_Interface && req.Request == GetLineCoding) {
		SetupIn(req.Length, (uint8_t*)&lineCoding);
	}

	if (req.RequestType == HtoD_Class_Interface && req.Request == SetLineCoding) {
		// Prepare to receive line coding data in ClassSetupData
		usb->classPendingData = true;
		EndPointTransfer(Direction::out, 0, req.Length);
	}
}


void CDCHandler::ClassSetupData(usbRequest& req, const uint8_t* data)
{
	// ClassSetup passes instruction to set line coding - this is the data portion where the line coding is transferred
	if (req.RequestType == HtoD_Class_Interface && req.Request == SetLineCoding) {
		lineCoding = *(LineCoding*)data;
	}
}


int32_t CDCHandler::ParseInt(const std::string_view cmd, const char precedingChar, const int32_t low, const int32_t high) {
	int32_t val = -1;
	const int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(&cmd[pos + 1], "0123456789-") > 0) {
		val = std::stoi(&cmd[pos + 1]);
	}
	if (high > low && (val > high || val < low)) {
		printf("Must be a value between %ld and %ld\r\n", low, high);
		return low - 1;
	}
	return val;
}


float CDCHandler::ParseFloat(const std::string_view cmd, const char precedingChar, const float low = 0.0f, const float high = 0.0f) {
	float val = -1.0f;
	const int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(&cmd[pos + 1], "0123456789.") > 0) {
		val = std::stof(&cmd[pos + 1]);
	}
	if (high > low && (val > high || val < low)) {
		//printf("Must be a value between %f and %f\r\n", low, high);		// FIXME - not enough space for printf floats
		printf("Invalid value\r\n");
		return low - 1.0f;
	}
	return val;
}


// Descriptor definition here as requires constants from USB class
const uint8_t CDCHandler::Descriptor[] = {
	// IAD Descriptor - Interface association descriptor for CDC class
	0x08,									// bLength (8 bytes)
	USBMain::IadDescriptor,					// bDescriptorType
	USBMain::CDCCmdInterface,				// bFirstInterface
	0x02,									// bInterfaceCount
	0x02,									// bFunctionClass (Communications and CDC Control)
	0x02,									// bFunctionSubClass
	0x01,									// bFunctionProtocol
	USBMain::CommunicationClass,			// String Descriptor

	// Interface Descriptor
	0x09,									// bLength: Interface Descriptor size
	USBMain::InterfaceDescriptor,			// bDescriptorType: Interface
	USBMain::CDCCmdInterface,				// bInterfaceNumber: Number of Interface
	0x00,									// bAlternateSetting: Alternate setting
	0x01,									// bNumEndpoints: 1 endpoint used
	0x02,									// bInterfaceClass: Communication Interface Class
	0x02,									// bInterfaceSubClass: Abstract Control Model
	0x01,									// bInterfaceProtocol: Common AT commands
	USBMain::CommunicationClass,			// iInterface

	// Header Functional Descriptor
	0x05,									// bLength: Endpoint Descriptor size
	USBMain::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x00,									// bDescriptorSubtype: Header Func Desc
	0x10,									// bcdCDC: spec release number
	0x01,

	// Call Management Functional Descriptor
	0x05,									// bFunctionLength
	USBMain::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x01,									// bDescriptorSubtype: Call Management Func Desc
	0x00,									// bmCapabilities: D0+D1
	0x01,									// bDataInterface: 1

	// ACM Functional Descriptor
	0x04,									// bFunctionLength
	USBMain::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x02,									// bDescriptorSubtype: Abstract Control Management desc
	0x02,									// bmCapabilities

	// Union Functional Descriptor
	0x05,									// bFunctionLength
	USBMain::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x06,									// bDescriptorSubtype: Union func desc
	0x00,									// bMasterInterface: Communication class interface
	0x01,									// bSlaveInterface0: Data Class Interface

	// Endpoint 2 Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USBMain::EndpointDescriptor,			// bDescriptorType: Endpoint
	USBMain::CDC_Cmd,						// bEndpointAddress
	USBMain::Interrupt,						// bmAttributes: Interrupt
	0x08,									// wMaxPacketSize
	0x00,
	0x10,									// bInterval

	//---------------------------------------------------------------------------

	// Data class interface descriptor
	0x09,									// bLength: Endpoint Descriptor size
	USBMain::InterfaceDescriptor,			// bDescriptorType:
	USBMain::CDCDataInterface,				// bInterfaceNumber: Number of Interface
	0x00,									// bAlternateSetting: Alternate setting
	0x02,									// bNumEndpoints: Two endpoints used
	0x0A,									// bInterfaceClass: CDC
	0x00,									// bInterfaceSubClass:
	0x00,									// bInterfaceProtocol:
	0x00,									// iInterface:

	// Endpoint OUT Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USBMain::EndpointDescriptor,			// bDescriptorType: Endpoint
	USBMain::CDC_Out,						// bEndpointAddress
	USBMain::Bulk,							// bmAttributes: Bulk
	LOBYTE(USBMain::ep_maxPacket),			// wMaxPacketSize:
	HIBYTE(USBMain::ep_maxPacket),
	0x00,									// bInterval: ignore for Bulk transfer

	// Endpoint IN Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USBMain::EndpointDescriptor,			// bDescriptorType: Endpoint
	USBMain::CDC_In,						// bEndpointAddress
	USBMain::Bulk,							// bmAttributes: Bulk
	LOBYTE(USBMain::ep_maxPacket),			// wMaxPacketSize:
	HIBYTE(USBMain::ep_maxPacket),
	0x00,									// bInterval: ignore for Bulk transfer
};


uint32_t CDCHandler::GetInterfaceDescriptor(const uint8_t** buffer) {
	*buffer = Descriptor;
	return sizeof(Descriptor);
}

