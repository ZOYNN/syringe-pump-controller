// ================================================================
//  test_command_parser.cpp
//  Tests CommandParser on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <cmath>
#include <iostream>

#include "command_parser.hpp"

// ========================== HELPERS ==========================

bool near(const float a, const float b)
{
	return std::fabs(a - b) < 0.0001F;
}

ParseStatus parse(const char* line, ScreenCommand& command)
{
	const CommandParser parser{};
	return parser.parse(line, command);
}

// ========================== MAIN ==========================

int main()
{
	ScreenCommand command{};

	// 1. commands with a value
	assert(parse("DIA19.20", command) == ParseStatus::ok);
	assert(command.type == CommandType::diameter);
	assert(near(command.value, 19.2F));

	assert(parse("FLW0.01", command) == ParseStatus::ok);
	assert(command.type == CommandType::flow);
	assert(near(command.value, 0.01F));

	assert(parse("FLW2", command) == ParseStatus::ok);
	assert(near(command.value, 2.0F));

	assert(parse("DIA9999.99", command) == ParseStatus::ok);
	assert(near(command.value, 9999.99F));

	// 1b. flow unit: one digit 0, 1 or 2
	assert(parse("UNT0", command) == ParseStatus::ok);
	assert(command.type == CommandType::unit);
	assert(command.unit == FlowUnit::ml_per_min);
	assert(parse("UNT1", command) == ParseStatus::ok);
	assert(command.unit == FlowUnit::ml_per_hour);
	assert(parse("UNT2\r", command) == ParseStatus::ok);
	assert(command.unit == FlowUnit::ul_per_min);
	assert(parse("UNT", command) == ParseStatus::bad_value);		// missing
	assert(parse("UNT3", command) == ParseStatus::bad_value);		// no such unit
	assert(parse("UNT12", command) == ParseStatus::bad_value);
	assert(parse("UNT1.0", command) == ParseStatus::bad_value);
	assert(parse("UNTx", command) == ParseStatus::bad_value);
	assert(command.type == CommandType::none);

	// 2. commands without a value
	assert(parse("RUN", command) == ParseStatus::ok);
	assert(command.type == CommandType::run);
	assert(parse("STP", command) == ParseStatus::ok);
	assert(command.type == CommandType::stop);
	assert(parse("JOG", command) == ParseStatus::ok);
	assert(command.type == CommandType::jog);
	assert(parse("JST", command) == ParseStatus::ok);
	assert(command.type == CommandType::jog_stop);

	// 3. '\r' and spaces at the end are OK
	assert(parse("RUN\r", command) == ParseStatus::ok);
	assert(parse("DIA19.2 ", command) == ParseStatus::ok);
	assert(near(command.value, 19.2F));

	// 4. empty lines
	assert(parse("", command) == ParseStatus::empty);
	assert(parse("\r", command) == ParseStatus::empty);
	assert(parse(nullptr, command) == ParseStatus::empty);

	// 5. unknown commands
	assert(parse("XYZ", command) == ParseStatus::unknown_command);
	assert(parse("run", command) == ParseStatus::unknown_command);
	assert(parse("RU", command) == ParseStatus::unknown_command);
	assert(parse("FWD", command) == ParseStatus::unknown_command);		// removed (one direction only)
	assert(parse("REV", command) == ParseStatus::unknown_command);
	assert(command.type == CommandType::none);

	// 6. bad values
	assert(parse("PAU", command) == ParseStatus::unknown_command);	// removed 2026-11-16
	assert(parse("DIA", command) == ParseStatus::bad_value);		// missing
	assert(parse("FLW1.2345", command) == ParseStatus::bad_value);	// 4 decimals
	assert(parse("FLW0.008", command) == ParseStatus::bad_value);   // 3 decimals
	assert(parse("DIA19.", command) == ParseStatus::bad_value);
	assert(parse("DIA.5", command) == ParseStatus::bad_value);
	assert(parse("DIA1.2.3", command) == ParseStatus::bad_value);
	assert(parse("DIA-1", command) == ParseStatus::bad_value);
	assert(parse("DIA1,5", command) == ParseStatus::bad_value);
	assert(parse("DIA12345", command) == ParseStatus::bad_value);	// 5 digits
	assert(parse("DIA 19", command) == ParseStatus::bad_value);
	assert(parse("RUN5", command) == ParseStatus::bad_value);		// RUN has no value
	assert(command.type == CommandType::none);

	std::cout << "command parser: ok\n";
	return 0;
}
