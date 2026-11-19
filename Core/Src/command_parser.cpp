// ================================================================
//  command_parser.cpp
//  Turns one text line from the screen into a command.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include "command_parser.hpp"

#include <cstring>

// ======================== HELPERS =========================

namespace
{

struct CommandName
{
	const char* name;
	CommandType type;
};

constexpr CommandName command_names[]{
	{"DIA", CommandType::diameter},
	{"FLW", CommandType::flow},
	{"UNT", CommandType::unit},
	{"RUN", CommandType::run},
	{"STP", CommandType::stop},
	{"JOG", CommandType::jog},
	{"JST", CommandType::jog_stop},
	{"FWD", CommandType::forward},
	{"REV", CommandType::reverse}
};

bool is_digit(const char c)
{
	return (c >= '0') && (c <= '9');
}

}	// namespace

// ======================== FUNCTIONS =======================

ParseStatus CommandParser::parse(const char* line, ScreenCommand& command) const
{
	command = ScreenCommand{};
	if (line == nullptr)
	{
		return ParseStatus::empty;
	}

	// '\r' and spaces at the end are not part of the command
	std::size_t length{std::strlen(line)};
	while ((length > 0U) && ((line[length - 1U] == '\r') || (line[length - 1U] == ' ')))
	{
		--length;
	}
	if (length == 0U)
	{
		return ParseStatus::empty;
	}

	if (length < name_length)
	{
		return ParseStatus::unknown_command;
	}
	const CommandType type{find_type(line)};
	if (type == CommandType::none)
	{
		return ParseStatus::unknown_command;
	}

	const char* value_text{line + name_length};
	const std::size_t value_length{length - name_length};

	if (type == CommandType::unit)
	{
		FlowUnit unit{FlowUnit::ml_per_min};
		if (!parse_unit(value_text, value_length, unit))
		{
			return ParseStatus::bad_value;		// e.g. "UNT", "UNT3", "UNT1.0"
		}
		command.type = type;
		command.unit = unit;
		return ParseStatus::ok;
	}

	if (!needs_value(type))
	{
		if (value_length != 0U)
		{
			return ParseStatus::bad_value;		// e.g. "RUN5"
		}
		command.type = type;
		return ParseStatus::ok;
	}

	float value{0.0F};
	if (!parse_value(value_text, value_length, value))
	{
		return ParseStatus::bad_value;
	}
	command.type = type;
	command.value = value;
	return ParseStatus::ok;
}

CommandType CommandParser::find_type(const char* line)
{
	for (const CommandName& entry : command_names)
	{
		if (std::strncmp(line, entry.name, name_length) == 0)
		{
			return entry.type;
		}
	}
	return CommandType::none;
}

bool CommandParser::needs_value(const CommandType type)
{
	return (type == CommandType::diameter) || (type == CommandType::flow);
}

// allowed: "19", "19.2", "19.20", "0.008"
// refused: "", ".5", "19.", "1.2345", "-1", "1,5", "12345", "1.2.3"
bool CommandParser::parse_value(const char* text, const std::size_t length, float& value)
{
	uint32_t whole{0U};			// digits before the dot
	uint32_t fraction{0U};		// digits after the dot
	uint32_t divider{1U};		// 10, 100 or 1000 after the dot
	std::size_t whole_digits{0U};
	std::size_t decimals{0U};
	bool dot_seen{false};

	for (std::size_t i{0U}; i < length; ++i)
	{
		const char c{text[i]};
		if (c == '.')
		{
			if (dot_seen || (whole_digits == 0U))
			{
				return false;		// second dot, or no digit before the dot
			}
			dot_seen = true;
		}
		else if (!is_digit(c))
		{
			return false;		// letter, '-', ',' ...
		}
		else if (!dot_seen)
		{
			if (whole_digits == max_whole_digits)
			{
				return false;
			}
			whole = (whole * 10U) + static_cast<uint32_t>(c - '0');
			++whole_digits;
		}
		else
		{
			if (decimals == max_decimals)
			{
				return false;
			}
			fraction = (fraction * 10U) + static_cast<uint32_t>(c - '0');
			divider *= 10U;
			++decimals;
		}
	}

	if ((whole_digits == 0U) || (dot_seen && (decimals == 0U)))
	{
		return false;		// no number, or "19."
	}

	value = static_cast<float>(whole) + (static_cast<float>(fraction) / static_cast<float>(divider));
	return true;
}

// allowed: exactly one digit 0, 1 or 2
bool CommandParser::parse_unit(const char* text, const std::size_t length, FlowUnit& unit)
{
	if (length != 1U)
	{
		return false;
	}
	switch (text[0])
	{
	case '0':
		unit = FlowUnit::ml_per_min;
		return true;
	case '1':
		unit = FlowUnit::ml_per_hour;
		return true;
	case '2':
		unit = FlowUnit::ul_per_min;
		return true;
	default:
		return false;
	}
}
