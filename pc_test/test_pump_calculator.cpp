// ================================================================
//  test_pump_calculator.cpp
//  Tests PumpCalculator on the PC, without the STM32.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-08
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <iostream>
#include <cmath>

#include "pump_calculator.hpp"

// ======================= PARAMETERS =======================

constexpr float test_ratio{10.0F / 21.0F};			// belt ratio
constexpr float tolerance_ratio{0.001F};			// 0.1%
constexpr float test_diameter_mm{19.2F};			// [mm]
constexpr float expected_area_mm2{289.53F};			// [mm^2]
constexpr float test_lead_mm{1.25F};				// [mm/turn]
constexpr float expected_pulses_per_mm{5376.0F};	// [pulses/mm]
constexpr float average_lead_mm{1.15F};				// [mm/turn]
constexpr float expected_pulses_per_ml{18568.1F};	// [pulses/mL]
constexpr float expected_step_hz{309.47F};			// [Hz]
constexpr float test_flow_ml_min{1.0F};				// [mL/min]
constexpr float test_volume_ml{20.0F};				// [mL]
constexpr float expected_target_pulses{371362.0F};	// [pulses]
constexpr float expected_duration_s{1200.0F};		// [s]
constexpr float slow_step_hz{1.5F};					// [Hz]
constexpr uint16_t expected_slow_prescaler{732U};
constexpr uint16_t expected_slow_auto_reload{65483U};
constexpr float fast_step_hz{18000.0F};				// [Hz]
constexpr uint16_t expected_fast_prescaler{0U};
constexpr uint16_t expected_fast_auto_reload{3999U};
constexpr float too_slow_flow_ml_min{0.00005F};		// [mL/min] gives 0.0155 Hz
constexpr float too_fast_flow_ml_min{200.0F};   	// [mL/min] gives 61,894 Hz
constexpr float slowest_step_hz{0.0168F};       	// [Hz]
constexpr uint16_t expected_slowest_prescaler{65394U};

// ========================== HELPERS ==========================

bool is_close(const float value, const float expected)
{
	return std::fabs(value - expected) <= expected * tolerance_ratio;
}

// ========================== MAIN ==========================

int main()
{
	const PumpCalculator calculator{average_lead_mm, test_ratio, false};

	assert(calculator.lead_mm() == average_lead_mm);
	assert(!calculator.lead_is_measured());

	const PumpCalculator rod_calculator{test_lead_mm, test_ratio, true};

	assert(is_close(rod_calculator.area_mm2(test_diameter_mm), expected_area_mm2));
	assert(is_close(rod_calculator.pulses_per_mm(), expected_pulses_per_mm));
	assert(is_close(rod_calculator.pulses_per_ml(test_diameter_mm), expected_pulses_per_ml));
	assert(is_close(rod_calculator.step_hz(test_diameter_mm, test_flow_ml_min), expected_step_hz));
	assert(is_close(static_cast<float>(rod_calculator.target_pulses(test_diameter_mm, test_volume_ml)), expected_target_pulses));
	assert(is_close(rod_calculator.duration_s(test_diameter_mm, test_volume_ml, test_flow_ml_min), expected_duration_s));
	assert(rod_calculator.volume_stop_enabled());
	assert(!calculator.volume_stop_enabled());

	const TimerSettings slow{rod_calculator.timer_settings(slow_step_hz)};
	assert(slow.prescaler == expected_slow_prescaler);
	assert(slow.auto_reload == expected_slow_auto_reload);

	const TimerSettings fast{rod_calculator.timer_settings(fast_step_hz)};
	assert(fast.prescaler == expected_fast_prescaler);
	assert(fast.auto_reload == expected_fast_auto_reload);

	assert(rod_calculator.check_speed(test_diameter_mm, test_flow_ml_min) == CalcStatus::ok);
	assert(rod_calculator.check_speed(0.0F, test_flow_ml_min) == CalcStatus::bad_diameter);
	assert(rod_calculator.check_speed(-1.0F, test_flow_ml_min) == CalcStatus::bad_diameter);
	assert(rod_calculator.check_speed(test_diameter_mm, 0.0F) == CalcStatus::bad_flow);
	assert(rod_calculator.check_speed(test_diameter_mm, too_slow_flow_ml_min) == CalcStatus::step_rate_too_low);
	assert(rod_calculator.check_speed(test_diameter_mm, too_fast_flow_ml_min) == CalcStatus::step_rate_too_high);

	const PumpCalculator bad_lead_calculator{0.0F, test_ratio, false};
	assert(bad_lead_calculator.check_speed(test_diameter_mm, test_flow_ml_min) == CalcStatus::bad_lead);

	assert(rod_calculator.check_volume(test_volume_ml) == CalcStatus::ok);
	assert(rod_calculator.check_volume(0.0F) == CalcStatus::bad_volume);

	const TimerSettings slowest{rod_calculator.timer_settings(slowest_step_hz)};
	assert(slowest.prescaler == expected_slowest_prescaler);

	const PumpCalculator bad_ratio_calculator{test_lead_mm, 0.0F, true};
	assert(bad_ratio_calculator.check_speed(test_diameter_mm, test_flow_ml_min) == CalcStatus::bad_lead);

	// flow units -> mL/min
	assert(is_close(PumpCalculator::ml_per_hour_to_ml_per_min(90.0F), 1.5F));
	assert(is_close(PumpCalculator::ul_per_min_to_ml_per_min(1500.0F), 1.5F));
	assert(is_close(PumpCalculator::to_ml_per_min(1.5F, FlowUnit::ml_per_min), 1.5F));
	assert(is_close(PumpCalculator::to_ml_per_min(90.0F, FlowUnit::ml_per_hour), 1.5F));
	assert(is_close(PumpCalculator::to_ml_per_min(1500.0F, FlowUnit::ul_per_min), 1.5F));
	// 19.20 mm: 0.06 µL/min is the slowest the timer can make (0.0186 Hz), 0.05 µL/min is too slow (0.0155 Hz)
	assert(rod_calculator.check_speed(test_diameter_mm, PumpCalculator::to_ml_per_min(0.06F, FlowUnit::ul_per_min)) == CalcStatus::ok);
	assert(rod_calculator.check_speed(test_diameter_mm, PumpCalculator::to_ml_per_min(0.05F, FlowUnit::ul_per_min)) == CalcStatus::step_rate_too_low);

	std::cout << "step 6: ok\n";
	return 0;
}
