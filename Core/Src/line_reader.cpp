// ================================================================
//  line_reader.cpp
//  Collects UART bytes (from the interrupt) into text lines.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "line_reader.hpp"

#include <cstring>

// ======================== FUNCTIONS =======================

void LineReader::push_byte(const uint8_t byte)
{
	const std::size_t next{(head_ + 1U) % buffer_size};
	if (next == tail_)
	{
		return;		// buffer full: the byte is lost
	}
	buffer_[head_] = byte;
	head_ = next;
}

LineStatus LineReader::get_line(char* line, const std::size_t size)
{
	uint8_t byte{0U};
	while (pop_byte(byte))
	{
		if (byte == '\n')
		{
			const bool was_too_long{line_too_long_};
			line_[line_length_] = '\0';
			line_length_ = 0U;
			line_too_long_ = false;

			if (was_too_long || (line == nullptr) || (size <= max_line_length))
			{
				return LineStatus::too_long;
			}
			std::strcpy(line, line_);
			return LineStatus::ready;
		}

		if (!is_text_byte(byte))
		{
			continue;		// not text: throw away
		}

		if (line_length_ < max_line_length)
		{
			line_[line_length_] = static_cast<char>(byte);
			++line_length_;
		}
		else
		{
			line_too_long_ = true;		// keep reading until '\n', then throw away
		}
	}
	return LineStatus::none;
}

bool LineReader::pop_byte(uint8_t& byte)
{
	if (tail_ == head_)
	{
		return false;		// empty
	}
	byte = buffer_[tail_];
	tail_ = (tail_ + 1U) % buffer_size;
	return true;
}


bool LineReader::is_text_byte(const uint8_t byte)
{
	const bool printable{(byte >= 0x20U) && (byte <= 0x7EU)};		// ' ' ... '~'
	return printable || (byte == '\r') || (byte == '\t');
}
