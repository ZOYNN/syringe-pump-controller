// ================================================================
//  command_handler.hpp
//  Takes one line from the screen, gives it to the pump and
//  writes the answer back to the screen.
//  No hardware here, so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

#ifndef SYRINGE_PUMP_B__COMMAND_HANDLER_HPP_
#define SYRINGE_PUMP_B__COMMAND_HANDLER_HPP_

// ======================== INCLUDES ========================

#include <cstddef>

#include "command_parser.hpp"
#include "pump_controller.hpp"
#include "screen_interface.hpp"
#include "settings_store.hpp"

// ========================= CLASS ==========================

class CommandHandler
{
public:
	// names of the objects on the Nextion page
	static constexpr const char* status_object{"t_status"};
	static constexpr const char* message_object{"t_msg"};
	static constexpr const char* diameter_object{"t_dia"};
	static constexpr const char* flow_object{"t_flw"};
	static constexpr const char* unit_object{"t_unit"};		// flow unit label, e.g. "mL/sa"
	static constexpr const char* diameter_input{"in_dia"};		// the boxes the user types in
	static constexpr const char* flow_input{"in_flw"};

	static constexpr std::size_t diameter_decimals{2U};		// "19.20"
	static constexpr std::size_t flow_decimals{2U};			// "0.08"

	CommandHandler(PumpController& pump, ScreenInterface& screen, SettingsStore& store);

	void handle_line(const char* line);
	void restore();		// at power on, before show_all(): last settings from the flash
	void show_all();		// at power on, and when the jog stopped by itself

private:
	void on_value(const ScreenCommand& command);
	void on_pump_command(CommandType type);
	void on_unit(FlowUnit unit);
	CalcStatus try_settings(float diameter_mm, float flow, FlowUnit unit);
	void show_state();
	void show_unit();
	void show_message(const char* text);
	void show_values(bool also_inputs);
	void save_settings();
	void show_value(const char* object, bool known, float value, std::size_t decimals);

	CommandParser parser_{};
	PumpController& pump_;
	ScreenInterface& screen_;
	SettingsStore& store_;

	float diameter_mm_{0.0F};
	float flow_{0.0F};						// the number on the screen, in flow_unit_
	FlowUnit flow_unit_{FlowUnit::ml_per_min};
	bool has_diameter_{false};
	bool has_flow_{false};
};

#endif // SYRINGE_PUMP_B__COMMAND_HANDLER_HPP_
