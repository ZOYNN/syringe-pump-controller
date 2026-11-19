// ================================================================
//  pump_calculator.hpp
//  Calculates the syringe pump values on the STM32.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-08
// ================================================================


#ifndef SYRINGE_PUMP_B__PUMP_CALCULATOR_HPP_
#define SYRINGE_PUMP_B__PUMP_CALCULATOR_HPP_

// ======================== INCLUDES ========================

#include <cstdint>

// ========================= STRUCTS ========================

enum class CalcStatus
{
	ok,
	bad_diameter,
	bad_volume,
	bad_flow,
	bad_lead,
	step_rate_too_low,
	step_rate_too_high
};

// flow unit chosen on the screen (the number is also the UNT command code)
enum class FlowUnit : uint8_t
{
	ml_per_min = 0U,	// mL/min (mL/dk)
	ml_per_hour = 1U,	// mL/h   (mL/sa)
	ul_per_min = 2U		// µL/min (µL/dk)
};

struct TimerSettings
{
	uint16_t prescaler{0U};		// PSC register
	uint16_t auto_reload{0U};	// ARR register
};

// ========================= CLASS ==========================

class PumpCalculator
{
public:
	PumpCalculator(float lead_mm, float rod_turns_per_motor_turn, bool lead_is_measured);

	float lead_mm() const;
	float rod_turns_per_motor_turn() const;
	bool lead_is_measured() const;
	float area_mm2(float diameter_mm) const;	// [mm^2]
	float pulses_per_mm() const;				// [pulses/mm]
	float pulses_per_ml(float diameter_mm) const;	// [pulses/mL]
	float step_hz(float diameter_mm, float flow_ml_min) const;	// [Hz]
	float step_hz_for_rpm(float motor_rpm) const;				// [Hz] fixed motor speed (jog)
	uint32_t target_pulses(float diameter_mm, float volume_ml) const;	// [pulses]
	float duration_s(float diameter_mm, float volume_ml, float flow_ml_min) const;	// [s]
	bool volume_stop_enabled() const;
	TimerSettings timer_settings(float step_hz) const;
	CalcStatus check_speed(float diameter_mm, float flow_ml_min) const;
	CalcStatus check_volume(float volume_ml) const;

	// flow units: the calculation works in mL/min, other units are changed first
	static float ml_per_hour_to_ml_per_min(float flow_ml_h);		// mL/h / 60
	static float ul_per_min_to_ml_per_min(float flow_ul_min);		// µL/min / 1000
	static float to_ml_per_min(float flow, FlowUnit unit);

private:
	float lead_mm_;	// [mm/turn]
	float rod_turns_per_motor_turn_;		// [turn/turn] belt ratio
	bool lead_is_measured_;
};

#endif // SYRINGE_PUMP_B__PUMP_CALCULATOR_HPP_
