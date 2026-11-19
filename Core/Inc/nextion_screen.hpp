// ================================================================
//  nextion_screen.hpp
//  The real screen: sends text commands to the Nextion over UART.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

#ifndef SYRINGE_PUMP_B__NEXTION_SCREEN_HPP_
#define SYRINGE_PUMP_B__NEXTION_SCREEN_HPP_

// ======================== INCLUDES ========================

#include "main.h"
#include "screen_interface.hpp"

// ========================= CLASS ==========================

class NextionScreen : public ScreenInterface
{
public:
	explicit NextionScreen(UART_HandleTypeDef& uart);

	void show(const char* object, const char* text) override;

private:
	UART_HandleTypeDef& uart_;		// UART to the screen (USART1)
};

#endif // SYRINGE_PUMP_B__NEXTION_SCREEN_HPP_
