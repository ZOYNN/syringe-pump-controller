// ================================================================
//  pump_calculator.cpp
//  Calculates the syringe pump values on the STM32.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-08
// ================================================================

// ======================== INCLUDES ========================

#include "pump_calculator.hpp"
#include <cmath>

// ======================= PARAMETERS =======================

constexpr float pi{3.1415927F};
constexpr float full_steps_per_turn{200.0F};	// [steps/turn]
constexpr float microsteps_per_step{16.0F};		// [microsteps/step]
constexpr float ml_to_mm3{1000.0F};				// [mm^3/mL]
constexpr float seconds_per_min{60.0F};			// [second/min]
constexpr float minutes_per_hour{60.0F};		// [min/h]
constexpr float ul_per_ml{1000.0F};				// [µL/mL]
constexpr float timer_clock_hz{72000000.0F};	// [Hz]
constexpr uint32_t timer_counts{65536U};		// [counts] 16-bit register
constexpr float min_step_hz{0.0168F};			// [Hz] slowest rate the 16-bit timer can make
constexpr float max_step_hz{20000.0F};			// [Hz] guess, find the real limit with the motor test

// ======================== CONSTRUCTOR =====================

PumpCalculator::PumpCalculator(const float lead_mm, const float rod_turns_per_motor_turn, const bool lead_is_measured)
	: lead_mm_{lead_mm}, rod_turns_per_motor_turn_{rod_turns_per_motor_turn}, lead_is_measured_{lead_is_measured}
{

}

// ======================== METHODS =========================

float PumpCalculator::lead_mm() const
{
	return lead_mm_;
}

bool PumpCalculator::lead_is_measured() const
{
	return lead_is_measured_;
}

float PumpCalculator::area_mm2(const float diameter_mm) const
{
	return pi * diameter_mm * diameter_mm / 4.0F;
}

float PumpCalculator::pulses_per_ml(const float diameter_mm) const
{
	return ml_to_mm3 / area_mm2(diameter_mm) * pulses_per_mm();
}

float PumpCalculator::step_hz(const float diameter_mm, const float flow_ml_min) const
{
	return pulses_per_ml(diameter_mm) * flow_ml_min / seconds_per_min;
}

float PumpCalculator::step_hz_for_rpm(const float motor_rpm) const
{
	return motor_rpm * full_steps_per_turn * microsteps_per_step / seconds_per_min;
}

uint32_t PumpCalculator::target_pulses(const float diameter_mm, const float volume_ml) const
{
	return static_cast<uint32_t>(std::lround(volume_ml * pulses_per_ml(diameter_mm)));
}

float PumpCalculator::duration_s(const float diameter_mm, const float volume_ml, const float flow_ml_min) const
{
	return static_cast<float>(target_pulses(diameter_mm, volume_ml)) / step_hz(diameter_mm, flow_ml_min);
}

bool PumpCalculator::volume_stop_enabled() const
{
	return lead_is_measured_;
}

float PumpCalculator::rod_turns_per_motor_turn() const
{
	return rod_turns_per_motor_turn_;
}

float PumpCalculator::pulses_per_mm() const
{
	const float mm_per_motor_turn{lead_mm_ * rod_turns_per_motor_turn_};	// [mm/turn]
	return full_steps_per_turn * microsteps_per_step / mm_per_motor_turn;
}

TimerSettings PumpCalculator::timer_settings(const float step_hz) const
{
	const uint32_t ticks{static_cast<uint32_t>(std::llround(timer_clock_hz / step_hz))};		// [timer ticks per pulse]
	const uint32_t prescaler{(ticks - 1U) / timer_counts};
	const uint32_t auto_reload{ticks / (prescaler + 1U) - 1U};

	TimerSettings settings{};
	settings.prescaler = static_cast<uint16_t>(prescaler);
	settings.auto_reload = static_cast<uint16_t>(auto_reload);
	return settings;
}

CalcStatus PumpCalculator::check_speed(const float diameter_mm, const float flow_ml_min) const
{
	if ((lead_mm_ <= 0.0F) || (rod_turns_per_motor_turn_ <= 0.0F))
	{
		return CalcStatus::bad_lead;
	}
	if (diameter_mm <= 0.0F)
	{
		return CalcStatus::bad_diameter;
	}
	if (flow_ml_min <= 0.0F)
	{
		return CalcStatus::bad_flow;
	}

	const float rate_hz{step_hz(diameter_mm, flow_ml_min)};	// [Hz]
	if (rate_hz < min_step_hz)
	{
		return CalcStatus::step_rate_too_low;
	}
	if (rate_hz > max_step_hz)
	{
		return CalcStatus::step_rate_too_high;
	}
	return CalcStatus::ok;
}

CalcStatus PumpCalculator::check_volume(const float volume_ml) const
{
	if (volume_ml <= 0.0F)
	{
		return CalcStatus::bad_volume;
	}
	return CalcStatus::ok;
}

float PumpCalculator::ml_per_hour_to_ml_per_min(const float flow_ml_h)
{
	return flow_ml_h / minutes_per_hour;
}

float PumpCalculator::ul_per_min_to_ml_per_min(const float flow_ul_min)
{
	return flow_ul_min / ul_per_ml;
}

float PumpCalculator::to_ml_per_min(const float flow, const FlowUnit unit)
{
	switch (unit)
	{
	case FlowUnit::ml_per_hour:
		return ml_per_hour_to_ml_per_min(flow);
	case FlowUnit::ul_per_min:
		return ul_per_min_to_ml_per_min(flow);
	case FlowUnit::ml_per_min:
		break;
	}
	return flow;
}
