// ================================================================
//  screen_text.hpp
//  The Turkish texts that the Blue Pill shows on the screen.
//  Texts are in iso-8859-9 (Turkish), like the Nextion project.
//  No hardware here, so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

#ifndef SYRINGE_PUMP_B__SCREEN_TEXT_HPP_
#define SYRINGE_PUMP_B__SCREEN_TEXT_HPP_

// ======================== INCLUDES ========================

#include <cstddef>

#include "command_parser.hpp"
#include "pump_controller.hpp"

// ========================= CLASS ==========================

class ScreenText
{
public:
	static constexpr std::size_t max_decimals{2U};		// same as CommandParser
	static constexpr float max_value{9999.99F};		// same as CommandParser

	static const char* state_text(PumpState state);
	static const char* direction_text(Direction direction);
	static const char* unit_text(FlowUnit unit);		// label next to the flow: "mL/dk" ...

	// "" when the status is ok (the message box is cleared)
	static const char* parse_error_text(ParseStatus status);
	static const char* calc_error_text(CalcStatus status);
	static const char* command_error_text(CommandStatus status);
	static const char* save_error_text();		// flash write failed

	// 19.2 with 2 decimals -> "19.20"; no float printf needed
	// returns false for a negative or too big value, or when out is too small
	static bool format_value(float value, std::size_t decimals, char* out, std::size_t size);
};

#endif // SYRINGE_PUMP_B__SCREEN_TEXT_HPP_
