// ================================================================
//  test_command_handler.cpp
//  Tests CommandHandler on the PC, with a fake motor and a fake screen.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <string>

#include "command_handler.hpp"
#include "fake_flash.hpp"
#include "screen_text.hpp"

// ======================= PARAMETERS =======================

constexpr float test_lead_mm{1.25F};		// [mm/turn]
constexpr float test_ratio{10.0F / 21.0F};				// belt ratio

// ======================== FAKE MOTOR ======================

class FakeMotor : public MotorInterface
{
public:
	void set_rate(const TimerSettings& settings) override
	{
		last_settings = settings;
	}

	void start() override
	{
		running = true;
	}

	void stop() override
	{
		running = false;
	}

	bool is_running() const override
	{
		return running;
	}

	bool running{false};
	TimerSettings last_settings{};
};

// ======================== FAKE SCREEN =====================

class FakeScreen : public ScreenInterface
{
public:
	void show(const char* object, const char* text) override
	{
		texts[object] = text;
		++show_count;
	}

	// last text of an object ("" when never shown)
	std::string text(const char* object)
	{
		return texts[object];
	}

	std::map<std::string, std::string> texts{};
	int show_count{0};
};

// ========================== MAIN ==========================

int main()
{
	const PumpCalculator calculator{test_lead_mm, test_ratio, true};
	FakeMotor motor{};
	PumpController pump{calculator, motor};
	FakeScreen screen{};
	FakeFlash flash{};
	SettingsStore store{flash, FakeFlash::base};
	CommandHandler handler{pump, screen, store};

	const char* status{CommandHandler::status_object};
	const char* message{CommandHandler::message_object};

	// 1. power on: state, empty message, no direction text (t_dir is gone)
	handler.show_all();
	assert(screen.text(status) == ScreenText::state_text(PumpState::idle));
	assert(screen.text(message) == "");
	assert(screen.texts.count("t_dir") == 0U);

	// 2. RUN without settings is refused
	handler.handle_line("RUN");
	assert(screen.text(message) == ScreenText::command_error_text(CommandStatus::no_settings));
	assert(screen.text(status) == ScreenText::state_text(PumpState::idle));
	assert(!motor.is_running());

	// 3. diameter, flow, run
	handler.handle_line("DIA19.20");
	assert(screen.text(CommandHandler::diameter_object) == "19.20");
	assert(screen.text(message) == "");
	handler.handle_line("FLW1");
	assert(screen.text(CommandHandler::flow_object) == "1.00");
	assert(screen.text(message) == "");
	handler.handle_line("RUN");
	assert(screen.text(message) == "");
	assert(screen.text(status) == ScreenText::state_text(PumpState::running));
	assert(motor.is_running());

	// 4. bad flow while running: error, the motor keeps going
	handler.handle_line("FLW0");
	assert(screen.text(message) == ScreenText::calc_error_text(CalcStatus::bad_flow));
	assert(motor.is_running());
	handler.handle_line("FLW1");
	assert(screen.text(message) == "");

	// 5. bad lines; an empty line changes nothing
	handler.handle_line("XYZ");
	assert(screen.text(message) == ScreenText::parse_error_text(ParseStatus::unknown_command));
	handler.handle_line("DIA1.2345");
	assert(screen.text(message) == ScreenText::parse_error_text(ParseStatus::bad_value));
	const int count_before{screen.show_count};
	handler.handle_line("");
	handler.handle_line("\r");
	assert(screen.show_count == count_before);
	assert(motor.is_running());

	// 6. FWD / REV are gone: unknown command, the motor keeps going, no t_dir
	handler.handle_line("REV");
	assert(screen.text(message) == ScreenText::parse_error_text(ParseStatus::unknown_command));
	assert(motor.is_running());
	handler.handle_line("FWD");
	assert(screen.text(message) == ScreenText::parse_error_text(ParseStatus::unknown_command));
	assert(screen.texts.count("t_dir") == 0U);
	handler.handle_line("STP");
	assert(screen.text(status) == ScreenText::state_text(PumpState::idle));
	assert(!motor.is_running());

	// 8. bad diameter (flow is known): error at once, the old diameter stays on the screen
	handler.handle_line("DIA0");
	assert(screen.text(CommandHandler::diameter_object) == "19.20");
	assert(screen.text(CommandHandler::diameter_input) == "19.20");
	assert(screen.text(message) == ScreenText::calc_error_text(CalcStatus::bad_diameter));

	// 9. flow too high while running: refused, old flow on the screen, motor keeps the old speed
	handler.handle_line("FLW2");
	handler.handle_line("RUN");
	assert(motor.is_running());
	const TimerSettings rate_before{motor.last_settings};
	handler.handle_line("FLW999");
	assert(screen.text(message) == ScreenText::calc_error_text(CalcStatus::step_rate_too_high));
	assert(screen.text(CommandHandler::flow_object) == "2.00");
	assert(screen.text(CommandHandler::flow_input) == "2.00");
	assert(motor.is_running());
	assert(motor.last_settings.prescaler == rate_before.prescaler);
	assert(motor.last_settings.auto_reload == rate_before.auto_reload);

	// 10. the refused flow is not kept: a new good diameter is accepted
	handler.handle_line("DIA10");
	assert(screen.text(message) == "");
	assert(screen.text(CommandHandler::diameter_object) == "10.00");
	assert(screen.text(CommandHandler::flow_object) == "2.00");
	handler.handle_line("STP");

	// 11. first pair refused (no old values): fields stay empty, RUN is refused
	FakeMotor motor2{};
	PumpController pump2{calculator, motor2};
	FakeScreen screen2{};
	FakeFlash flash2{};
	SettingsStore store2{flash2, FakeFlash::base};
	CommandHandler handler2{pump2, screen2, store2};
	handler2.handle_line("DIA19.20");
	handler2.handle_line("FLW999");
	assert(screen2.text(message) == ScreenText::calc_error_text(CalcStatus::step_rate_too_high));
	assert(screen2.text(CommandHandler::diameter_object) == "19.20");
	assert(screen2.text(CommandHandler::flow_object) == "");
	assert(screen2.text(CommandHandler::flow_input) == "");
	handler2.handle_line("RUN");
	assert(screen2.text(message) == ScreenText::command_error_text(CommandStatus::no_settings));
	assert(!motor2.is_running());

	// 12. JOG works without settings; repeated JOG does not write to the screen
	handler2.handle_line("JOG");
	assert(screen2.text(status) == ScreenText::state_text(PumpState::jogging));
	assert(motor2.is_running());
	const int count_jog{screen2.show_count};
	handler2.handle_line("JOG");
	assert(screen2.show_count == count_jog);
	handler2.handle_line("JST");
	assert(screen2.text(status) == ScreenText::state_text(PumpState::idle));
	assert(!motor2.is_running());

	// 13. JOG while running is refused, and its JST does not stop the run
	handler.handle_line("RUN");
	handler.handle_line("JOG");
	assert(screen.text(message) == ScreenText::command_error_text(CommandStatus::busy));
	assert(screen.text(status) == ScreenText::state_text(PumpState::running));
	handler.handle_line("JST");
	assert(screen.text(status) == ScreenText::state_text(PumpState::running));
	assert(screen.text(message) == ScreenText::command_error_text(CommandStatus::busy));	// message stays
	assert(motor.is_running());
	handler.handle_line("STP");

	// 14. power cut: a new handler on the same flash gets the last values back,
	//     shows them (also in the input boxes), and the pump does NOT start
	handler.handle_line("DIA19.20");
	handler.handle_line("FLW1.5");
	FakeMotor motor3{};
	PumpController pump3{calculator, motor3};
	FakeScreen screen3{};
	SettingsStore store3{flash, FakeFlash::base};
	CommandHandler handler3{pump3, screen3, store3};
	handler3.restore();
	handler3.show_all();
	assert(screen3.text(CommandHandler::diameter_object) == "19.20");
	assert(screen3.text(CommandHandler::flow_object) == "1.50");
	assert(screen3.text(CommandHandler::diameter_input) == "19.20");
	assert(screen3.text(CommandHandler::flow_input) == "1.50");
	assert(screen3.texts.count("t_dir") == 0U);
	assert(screen3.text(status) == ScreenText::state_text(PumpState::idle));
	assert(!motor3.is_running());
	handler3.handle_line("RUN");			// values are really in the pump
	assert(motor3.is_running());
	handler3.handle_line("STP");

	// 15. a refused value is not saved
	handler3.handle_line("FLW999");
	SettingsStore store4{flash, FakeFlash::base};
	SavedSettings saved{};
	assert(store4.load(saved));
	assert(std::fabs(saved.flow - 1.5F) < 0.001F);

	// 16. flash error: message KAYIT HATASI, the value is still used
	flash.fail = true;
	handler3.handle_line("FLW2");
	assert(screen3.text(message) == ScreenText::save_error_text());
	assert(screen3.text(CommandHandler::flow_object) == "2.00");
	flash.fail = false;

	// 17. empty flash at power on: fields stay empty
	FakeFlash empty_flash{};
	SettingsStore empty_store{empty_flash, FakeFlash::base};
	FakeMotor motor5{};
	PumpController pump5{calculator, motor5};
	FakeScreen screen5{};
	CommandHandler handler5{pump5, screen5, empty_store};
	handler5.restore();
	handler5.show_all();
	assert(screen5.text(CommandHandler::diameter_object) == "");

	// 18. flow units: power on shows mL/dk; the unit can be chosen before the values
	FakeFlash unit_flash{};
	SettingsStore unit_store{unit_flash, FakeFlash::base};
	FakeMotor motor6{};
	PumpController pump6{calculator, motor6};
	FakeScreen screen6{};
	CommandHandler handler6{pump6, screen6, unit_store};
	const char* unit{CommandHandler::unit_object};
	handler6.restore();
	handler6.show_all();
	assert(screen6.text(unit) == ScreenText::unit_text(FlowUnit::ml_per_min));
	handler6.handle_line("UNT1");
	assert(screen6.text(unit) == ScreenText::unit_text(FlowUnit::ml_per_hour));
	assert(screen6.text(message) == "");

	// 19. 90 mL/h = 1.5 mL/min: the motor gets the speed of 1.5 mL/min
	handler6.handle_line("DIA19.20");
	handler6.handle_line("FLW90");
	assert(screen6.text(CommandHandler::flow_object) == "90.00");
	handler6.handle_line("RUN");
	assert(motor6.is_running());
	const TimerSettings rate_1_5{calculator.timer_settings(calculator.step_hz(19.2F, 1.5F))};
	assert(motor6.last_settings.prescaler == rate_1_5.prescaler);
	assert(motor6.last_settings.auto_reload == rate_1_5.auto_reload);

	// 20. unit change while running: 90 mL/min is too fast -> refused, old unit and speed stay
	handler6.handle_line("UNT0");
	assert(screen6.text(message) == ScreenText::calc_error_text(CalcStatus::step_rate_too_high));
	assert(screen6.text(unit) == ScreenText::unit_text(FlowUnit::ml_per_hour));
	assert(motor6.last_settings.prescaler == rate_1_5.prescaler);
	assert(motor6.last_settings.auto_reload == rate_1_5.auto_reload);
	assert(motor6.is_running());

	// 21. unit change while running: 90 µL/min = 0.09 mL/min -> new speed at once
	handler6.handle_line("UNT2");
	assert(screen6.text(message) == "");
	assert(screen6.text(unit) == ScreenText::unit_text(FlowUnit::ul_per_min));
	assert(screen6.text(CommandHandler::flow_object) == "90.00");		// the number stays
	const TimerSettings rate_0_09{calculator.timer_settings(calculator.step_hz(19.2F, 0.09F))};
	assert(motor6.last_settings.prescaler == rate_0_09.prescaler);
	assert(motor6.last_settings.auto_reload == rate_0_09.auto_reload);
	assert(motor6.is_running());

	// 22. a bad unit command changes nothing
	handler6.handle_line("UNT3");
	assert(screen6.text(message) == ScreenText::parse_error_text(ParseStatus::bad_value));
	assert(screen6.text(unit) == ScreenText::unit_text(FlowUnit::ul_per_min));
	handler6.handle_line("UNT1");
	handler6.handle_line("STP");

	// 23. power cut: the unit comes back, and the pump gets the right speed
	FakeMotor motor7{};
	PumpController pump7{calculator, motor7};
	FakeScreen screen7{};
	SettingsStore unit_store2{unit_flash, FakeFlash::base};
	CommandHandler handler7{pump7, screen7, unit_store2};
	handler7.restore();
	handler7.show_all();
	assert(screen7.text(unit) == ScreenText::unit_text(FlowUnit::ml_per_hour));
	assert(screen7.text(CommandHandler::flow_object) == "90.00");
	assert(screen7.text(CommandHandler::flow_input) == "90.00");
	handler7.handle_line("RUN");
	assert(motor7.is_running());
	assert(motor7.last_settings.prescaler == rate_1_5.prescaler);
	assert(motor7.last_settings.auto_reload == rate_1_5.auto_reload);

	// 24. a new flow is read in the chosen unit (19.20 mm): 0.06 µL/min accepted, 0.05 µL/min too slow
	handler7.handle_line("STP");
	handler7.handle_line("UNT2");
	handler7.handle_line("FLW0.06");
	assert(screen7.text(message) == "");
	assert(screen7.text(CommandHandler::flow_object) == "0.06");
	handler7.handle_line("FLW0.05");
	assert(screen7.text(message) == ScreenText::calc_error_text(CalcStatus::step_rate_too_low));
	assert(screen7.text(CommandHandler::flow_object) == "0.06");

	std::cout << "command handler: ok\n";
	return 0;
}
