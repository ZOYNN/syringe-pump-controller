// ================================================================
//  line_reader.hpp
//  Collects UART bytes (from the interrupt) into text lines.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__LINE_READER_HPP_
#define SYRINGE_PUMP_B__LINE_READER_HPP_

// ======================== INCLUDES ========================

#include <cstddef>
#include <cstdint>

// ========================= STRUCTS ========================

enum class LineStatus
{
	none,		// no full line yet
	ready,		// a full line is in the output
	too_long	// a line was too long, it is thrown away
};

// ========================= CLASS ==========================

class LineReader
{
public:
	static constexpr std::size_t max_line_length{32U};	// [chars] without '\0'

	// called in the UART interrupt, must be short
	void push_byte(uint8_t byte);

	// called in app_loop; line must have room for max_line_length + 1 chars
	// bytes that are not text (e.g. 0x00, 0xFF, 0x88 from the Nextion) are thrown away
	LineStatus get_line(char* line, std::size_t size);

private:
	static constexpr std::size_t buffer_size{64U};	// ring buffer [bytes]

	bool pop_byte(uint8_t& byte);
	static bool is_text_byte(uint8_t byte);

	// ring buffer: head_ is written only by the interrupt, tail_ only by the loop
	volatile uint8_t buffer_[buffer_size]{};
	volatile std::size_t head_{0U};
	volatile std::size_t tail_{0U};

	// line that is being built
	char line_[max_line_length + 1U]{};
	std::size_t line_length_{0U};
	bool line_too_long_{false};
};

#endif	// SYRINGE_PUMP_B__LINE_READER_HPP_
