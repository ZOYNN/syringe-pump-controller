// ================================================================
//  command_parser.hpp
//  Turns one text line from the screen into a command.
//  Example lines: "DIA19.20", "FLW0.08", "UNT1", "RUN", "REV".
//  No hardware here, so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

#ifndef SYRINGE_PUMP_B__COMMAND_PARSER_HPP_
#define SYRINGE_PUMP_B__COMMAND_PARSER_HPP_

// ======================== INCLUDES ========================

#include <cstddef>
#include <cstdint>

#include "pump_calculator.hpp"

// ========================= TYPES ==========================

enum class CommandType
{
	none,
	diameter,	// DIA<value> [mm]
	flow,		// FLW<value> [in the chosen unit]
	unit,		// UNT0 = mL/min, UNT1 = mL/h, UNT2 = µL/min
	run,		// RUN
	stop,		// STP
	jog,		// JOG (İLERİ SÜR held)
	jog_stop,	// JST (İLERİ SÜR released): stops only a jog
	forward,	// FWD
	reverse		// REV
};

enum class ParseStatus
{
	ok,
	empty,				// empty line: nothing to do
	unknown_command,	// first 3 letters are not a known command
	bad_value			// value missing, not allowed, or wrong format
};

struct ScreenCommand
{
	CommandType type{CommandType::none};
	float value{0.0F};		// only for diameter and flow
	FlowUnit unit{FlowUnit::ml_per_min};	// only for unit
};

// ========================= CLASS ==========================

class CommandParser
{
public:
	static constexpr std::size_t name_length{3U};		// "DIA", "RUN" ...
	static constexpr std::size_t max_whole_digits{4U};	// before the dot: max 9999
	static constexpr std::size_t max_decimals{2U};		// after the dot: max 2 digits

	ParseStatus parse(const char* line, ScreenCommand& command) const;

private:
	static CommandType find_type(const char* line);
	static bool needs_value(CommandType type);
	static bool parse_value(const char* text, std::size_t length, float& value);
	static bool parse_unit(const char* text, std::size_t length, FlowUnit& unit);
};

#endif // SYRINGE_PUMP_B__COMMAND_PARSER_HPP_
