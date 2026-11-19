// ================================================================
//  app.cpp
//  Entry points of the C++ application, called from main.c.
//  Makes the objects and handles the commands from the screen.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "app.hpp"

#include "main.h"
#include "command_handler.hpp"
#include "line_reader.hpp"
#include "nextion_screen.hpp"
#include "pump_calculator.hpp"
#include "pump_controller.hpp"
#include "settings_store.hpp"
#include "stepper_driver.hpp"
#include "stm_flash.hpp"

// ======================= PARAMETERS =======================

constexpr float lead_mm{1.25F};		// [mm/turn] M8 rod (Papa, 2026-11-16)
constexpr float rod_turns_per_motor_turn{10.0F / 21.0F};	// belt: motor 10 teeth, rod 21 teeth
constexpr bool lead_is_measured{true};
constexpr uint32_t settings_address{0x0800F800U};	// last 2 flash pages (linker: FLASH = 62K)
constexpr uint32_t screen_start_ms{1000U};	// [ms] wait until the screen has started

// ========================= OBJECTS ========================

namespace
{
PumpCalculator calculator{lead_mm, rod_turns_per_motor_turn, lead_is_measured};
StepperDriver driver{htim3};
PumpController pump{calculator, driver};
NextionScreen screen{huart1};
StmFlash flash;
SettingsStore store{flash, settings_address};
CommandHandler handler{pump, screen, store};
LineReader reader;
uint8_t rx_byte{0U};		// the UART interrupt writes the new byte here


// ========================= HELPERS ========================

void start_receive()
{
	HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);		// wait for the next byte
}

void read_screen()
{
	char line[LineReader::max_line_length + 1U]{};
	if (reader.get_line(line, sizeof(line)) == LineStatus::ready)
	{
		handler.handle_line(line);		// too long lines are thrown away (no command is that long)
	}
}


} //namespace
// ======================== FUNCTIONS =======================

void app_setup(void)
{
	start_receive();
	HAL_Delay(screen_start_ms);		// screen needs time to start and to set 115200 baud
	handler.restore();				// last diameter, flow, direction (pump does not start)
	handler.show_all();				// first texts on the screen
}

void app_loop(void)
{
	if (pump.update(HAL_GetTick()))
	{
		handler.show_all();		// jog stopped by itself (no JOG from the screen)
	}
	read_screen();
}

// ======================== INTERRUPTS ======================

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == USART1)
	{
		reader.push_byte(rx_byte);
		start_receive();
	}
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == USART1)
	{
		start_receive();		// after an error (e.g. overrun) HAL stops receiving: start again
	}
}
