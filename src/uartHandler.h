#include "initialisation.h"
#include <string>
//#include <sstream>
#include <iomanip>

extern uint8_t uartCmdPos;
extern char uartCmd[100];
extern bool uartCmdRdy;
extern bool toggleSaw;

std::string IntToString(const int32_t& v);
std::string HexToString(const uint32_t& v, const bool& spaces);
std::string HexByte(const uint16_t& v);
void uartSendString(const std::string& s);
void uartSendString(const char* s);
void InitUART();
void uartCommand();
