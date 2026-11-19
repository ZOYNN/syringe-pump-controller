// ================================================================
//  nextion_command.cpp
//  Makes Nextion screen commands (text + FF FF FF at the end).
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "nextion_command.hpp"

#include <cstdio>
#include <cstring>

// ======================= PARAMETERS =======================

constexpr std::size_t end_byte_count{3U};		// every command ends with FF FF FF

// ======================== FUNCTIONS =======================

std::size_t NextionCommand::make_text(const char* object, const char* text,
									  char* out, const std::size_t size)
{
	if ((object == nullptr) || (text == nullptr) || (out == nullptr))
	{
		return 0U;
	}
	if (std::strchr(text, '"') != nullptr)
	{
		return 0U;
	}

	const int written{std::snprintf(out, size, "%s.txt=\"%s\"", object, text)};
	if (written < 0)
	{
		return 0U;
	}

	const std::size_t length{static_cast<std::size_t>(written)};
	if ((length + end_byte_count) > size)
	{
		return 0U;		// no room for the text or for FF FF FF
	}

	for (std::size_t i = 0U; i < end_byte_count; ++i)
	{
		out[length + i] = static_cast<char>(0xFF);
	}
	return length + end_byte_count;
}
