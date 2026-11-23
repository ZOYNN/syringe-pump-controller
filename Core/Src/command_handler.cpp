// ================================================================
//  command_handler.cpp
//  Takes one line from the screen, gives it to the pump and
//  writes the answer back to the screen.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include "command_handler.hpp"

#include "screen_text.hpp"

// ======================== CONSTRUCTOR =====================

CommandHandler::CommandHandler(PumpController& pump, ScreenInterface& screen, SettingsStore& store)
	: pump_{pump}, screen_{screen}, store_{store}
{

}

// ======================== METHODS =========================

void CommandHandler::handle_line(const char* line)
{
	ScreenCommand command{};
	const ParseStatus status{parser_.parse(line, command)};
	if (status == ParseStatus::empty)
	{
		return;		// nothing to do
	}
	if (status != ParseStatus::ok)
	{
		show_message(ScreenText::parse_error_text(status));
		return;
	}

	switch (command.type)
	{
	case CommandType::diameter:
	case CommandType::flow:
		on_value(command);
		break;
	case CommandType::unit:
		on_unit(command.unit);
		break;
	case CommandType::run:
	case CommandType::stop:
	case CommandType::jog:
	case CommandType::jog_stop:
		on_pump_command(command.type);
		break;
	case CommandType::none:
		break;
	}
}

// the pump never starts by itself: the state stays HAZIR
void CommandHandler::restore()
{
	SavedSettings saved{};
	if (!store_.load(saved))
	{
		return;		// nothing saved yet: start with empty fields
	}

	flow_unit_ = saved.flow_unit;
	if (saved.has_values && (try_settings(saved.diameter_mm, saved.flow, saved.flow_unit) == CalcStatus::ok))
	{
		diameter_mm_ = saved.diameter_mm;
		flow_ = saved.flow;
		has_diameter_ = true;
		has_flow_ = true;
	}
}

void CommandHandler::show_all()
{
	show_state();
	show_unit();
	show_values(true);		// also the input boxes (values from restore())
	show_message("");
}

// a new value is used only when the pump accepts it;
// a refused value is not saved, the screen shows the old values again
void CommandHandler::on_value(const ScreenCommand& command)
{
	float new_diameter_mm{diameter_mm_};
	float new_flow{flow_};
	bool new_has_diameter{has_diameter_};
	bool new_has_flow{has_flow_};
	if (command.type == CommandType::diameter)
	{
		new_diameter_mm = command.value;
		new_has_diameter = true;
	}
	else
	{
		new_flow = command.value;
		new_has_flow = true;
	}

	// both known: the pump checks them (bad values: the pump keeps the old ones)
	if (new_has_diameter && new_has_flow)
	{
		const CalcStatus status{try_settings(new_diameter_mm, new_flow, flow_unit_)};
		if (status != CalcStatus::ok)
		{
			show_values(true);		// old values back, also in the input boxes
			show_message(ScreenText::calc_error_text(status));
			return;
		}
	}

	// accepted (or only one value known yet: wait for the other one)
	diameter_mm_ = new_diameter_mm;
	flow_ = new_flow;
	has_diameter_ = new_has_diameter;
	has_flow_ = new_has_flow;
	show_values(false);
	show_message("");
	save_settings();
}

void CommandHandler::on_pump_command(const CommandType type)
{
	const PumpState before{pump_.state()};
	CommandStatus status{CommandStatus::ok};
	if (type == CommandType::run)
	{
		status = pump_.start();
	}
	else if (type == CommandType::jog)
	{
		status = pump_.jog();
		if ((status == CommandStatus::ok) && (before == PumpState::jogging))
		{
			return;		// JOG repeats every 200 ms: no screen update needed
		}
	}
	else if (type == CommandType::jog_stop)
	{
		pump_.stop_jog();
		if (before != PumpState::jogging)
		{
			return;		// no jog was running (e.g. JOG was refused): nothing to show
		}
	}
	else
	{
		pump_.stop();
	}

	show_message(ScreenText::command_error_text(status));
	show_state();
}

// The number stays, only the unit changes: 1.50 mL/dk -> 1.50 mL/sa.
// So the motor speed changes like after a new FLW (also while running).
// A unit that gives a refused speed is not used: the old unit stays.
void CommandHandler::on_unit(const FlowUnit unit)
{
	if (has_diameter_ && has_flow_)
	{
		const CalcStatus status{try_settings(diameter_mm_, flow_, unit)};
		if (status != CalcStatus::ok)
		{
			show_unit();			// old unit back on the screen
			show_message(ScreenText::calc_error_text(status));
			return;
		}
	}

	flow_unit_ = unit;
	show_unit();
	show_message("");
	save_settings();
}

// the pump always gets mL/min
CalcStatus CommandHandler::try_settings(const float diameter_mm, const float flow, const FlowUnit unit)
{
	return pump_.set_settings(diameter_mm, PumpCalculator::to_ml_per_min(flow, unit));
}

// only the accepted values; on a flash error the pump still works, the values are only not kept
void CommandHandler::save_settings()
{
	SavedSettings settings{};
	settings.has_values = has_diameter_ && has_flow_;
	settings.diameter_mm = diameter_mm_;
	settings.flow = flow_;
	settings.flow_unit = flow_unit_;
	if (!store_.save(settings))
	{
		show_message(ScreenText::save_error_text());
	}
}

void CommandHandler::show_state()
{
	screen_.show(status_object, ScreenText::state_text(pump_.state()));
}

void CommandHandler::show_unit()
{
	screen_.show(unit_object, ScreenText::unit_text(flow_unit_));
}

void CommandHandler::show_message(const char* text)
{
	screen_.show(message_object, text);
}

void CommandHandler::show_values(const bool also_inputs)
{
	show_value(diameter_object, has_diameter_, diameter_mm_, diameter_decimals);
	show_value(flow_object, has_flow_, flow_, flow_decimals);
	if (also_inputs)
	{
		show_value(diameter_input, has_diameter_, diameter_mm_, diameter_decimals);
		show_value(flow_input, has_flow_, flow_, flow_decimals);
	}
}

// unknown value: empty text
void CommandHandler::show_value(const char* object, const bool known, const float value, const std::size_t decimals)
{
	char text[16]{};
	if (known && !ScreenText::format_value(value, decimals, text, sizeof(text)))
	{
		return;		// cannot happen with values from the parser (max 9999.999)
	}
	screen_.show(object, text);
}
