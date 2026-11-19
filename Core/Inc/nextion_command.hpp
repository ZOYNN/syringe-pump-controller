// ================================================================
//  nextion_command.hpp
//  Makes Nextion screen commands (text + FF FF FF at the end).
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__NEXTION_COMMAND_HPP_
#define SYRINGE_PUMP_B__NEXTION_COMMAND_HPP_

// ======================== INCLUDES ========================

#include <cstddef>

// ========================= CLASS ==========================

class NextionCommand
{
public:
	// makes: <object>.txt="<text>" + 0xFF 0xFF 0xFF
	// returns the number of bytes in out, or 0 when it does not fit
	// or when text has a '"' (the screen cannot read it)
	static std::size_t make_text(const char* object, const char* text,
								 char* out, std::size_t size);
};

#endif	// SYRINGE_PUMP_B__NEXTION_COMMAND_HPP_
