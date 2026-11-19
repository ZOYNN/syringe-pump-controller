// ================================================================
//  nextion_screen.cpp
//  The real screen: sends text commands to the Nextion over UART.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include "nextion_screen.hpp"

#include "nextion_command.hpp"

// ======================= PARAMETERS =======================

constexpr uint32_t uart_send_timeout_ms{100U};		// [ms]
constexpr std::size_t command_size{64U};			// longest command + FF FF FF

// ======================== CONSTRUCTOR =====================

NextionScreen::NextionScreen(UART_HandleTypeDef& uart) : uart_{uart}
{

}

// ======================== METHODS =========================

void NextionScreen::show(const char* object, const char* text)
{
	char command[command_size]{};
	const std::size_t length{NextionCommand::make_text(object, text, command, sizeof(command))};
	if (length == 0U)
	{
		return;		// does not fit: send nothing
	}
	HAL_UART_Transmit(&uart_, reinterpret_cast<const uint8_t*>(command),
					  static_cast<uint16_t>(length), uart_send_timeout_ms);
}
