// ================================================================
//  pump_controller.cpp
//  The logic of the pump: settings, start, stop, direction, jog.
//  No hardware here, so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "pump_controller.hpp"

// ======================== CONSTRUCTOR =====================

PumpController::PumpController(const PumpCalculator& calculator, MotorInterface& motor) : calculator_{calculator}, motor_{motor}
{

}

// ======================== METHODS =========================

CalcStatus PumpController::set_settings(const float diameter_mm, const float flow_ml_min)
{
	const CalcStatus status{calculator_.check_speed(diameter_mm, flow_ml_min)};
	if (status != CalcStatus::ok)
	{
		return status;		// bad values: keep the old settings
	}

	timer_ = calculator_.timer_settings(calculator_.step_hz(diameter_mm, flow_ml_min));
	settings_valid_ = true;

	if (state_ == PumpState::running)
	{
		motor_.set_rate(timer_);		// new speed at once
	}
	return CalcStatus::ok;
}

CommandStatus PumpController::start()
{
	if (state_ == PumpState::jogging)
	{
		return CommandStatus::busy;		// first release İLERİ SÜR
	}
	if (!settings_valid_)
	{
		return CommandStatus::no_settings;
	}
	if (state_ == PumpState::running)
	{
		return CommandStatus::ok;		// already running
	}

	motor_.set_rate(timer_);
	motor_.start();
	state_ = PumpState::running;
	return CommandStatus::ok;
}

void PumpController::stop()
{
	motor_.stop();
	state_ = PumpState::idle;
}

// İLERİ SÜR: fixed speed, in the chosen direction (İLERİ or GERİ), needs no diameter / flow.
// The DIR pin is not touched here: set_direction() already set it (FWD / REV, or restore at power on).
// The screen sends JOG again every 200 ms while the button is held.
CommandStatus PumpController::jog()
{
	if (state_ == PumpState::running)
	{
		return CommandStatus::busy;
	}

	jog_refreshed_ = true;
	if (state_ == PumpState::jogging)
	{
		return CommandStatus::ok;		// already jogging: only keep it alive
	}

	jog_now_rpm_ = jog_start_rpm;		// start slow, update() makes it faster
	ramp_clock_set_ = false;
	motor_.set_rate(calculator_.timer_settings(calculator_.step_hz_for_rpm(jog_now_rpm_)));
	motor_.start();
	state_ = PumpState::jogging;
	return CommandStatus::ok;
}

// İLERİ SÜR released: a run started with BAŞLAT must not stop here
void PumpController::stop_jog()
{
	if (state_ == PumpState::jogging)
	{
		stop();
	}
}

// safety: if JST from the screen is lost, the jog stops after jog_timeout_ms
bool PumpController::update(const uint32_t now_ms)
{
	if (state_ != PumpState::jogging)
	{
		return false;
	}
	ramp_jog(now_ms);
	if (jog_refreshed_)
	{
		jog_refreshed_ = false;
		last_jog_ms_ = now_ms;
		return false;
	}
	if ((now_ms - last_jog_ms_) >= jog_timeout_ms)
	{
		stop();
		return true;
	}
	return false;
}

// a stepper can not jump from 0 to 120 rpm: it loses steps and may even turn backward.
// So the jog starts at jog_start_rpm and gets faster by jog_ramp_step_rpm every jog_ramp_interval_ms.
void PumpController::ramp_jog(const uint32_t now_ms)
{
	if (jog_now_rpm_ >= jog_rpm)
	{
		return;		// full speed reached
	}
	if (!ramp_clock_set_)
	{
		ramp_clock_set_ = true;		// first update() after jog(): start the ramp clock
		last_ramp_ms_ = now_ms;
		return;
	}
	if ((now_ms - last_ramp_ms_) < jog_ramp_interval_ms)
	{
		return;
	}

	last_ramp_ms_ = now_ms;
	jog_now_rpm_ += jog_ramp_step_rpm;
	if (jog_now_rpm_ > jog_rpm)
	{
		jog_now_rpm_ = jog_rpm;
	}
	motor_.set_rate(calculator_.timer_settings(calculator_.step_hz_for_rpm(jog_now_rpm_)));
}

// changed only when the motor stands (idle); while running or jogging: busy
CommandStatus PumpController::set_direction(const Direction direction)
{
	if (state_ != PumpState::idle)
	{
		return CommandStatus::busy;
	}

	motor_.set_direction(direction);
	direction_ = direction;
	return CommandStatus::ok;
}

PumpState PumpController::state() const
{
	return state_;
}

Direction PumpController::direction() const
{
	return direction_;
}

