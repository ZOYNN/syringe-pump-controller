// ================================================================
//  pump_controller.hpp
//  The logic of the pump: settings, start, stop, jog (one direction only).
//  No hardware here, so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__PUMP_CONTROLLER_HPP_
#define SYRINGE_PUMP_B__PUMP_CONTROLLER_HPP_

// ======================== INCLUDES ========================

#include <cstdint>

#include "motor_interface.hpp"
#include "pump_calculator.hpp"

// ========================= TYPES ==========================

enum class PumpState
{
	idle,
	running,
	jogging			// İLERİ SÜR button is held
};

enum class CommandStatus
{
	ok,
	no_settings,
	busy					// not allowed while the motor turns
};

// ========================= CLASS ==========================

class PumpController
{
public:
	static constexpr float jog_rpm{120.0F};				// [rpm] motor; 1.2 mm/s with the 10/21 belt
	static constexpr float jog_start_rpm{20.0F};			// [rpm] jog starts slow (a sudden 120 rpm start lost steps)
	static constexpr float jog_ramp_step_rpm{10.0F};		// [rpm] added every jog_ramp_interval_ms
	static constexpr uint32_t jog_ramp_interval_ms{50U};	// [ms] 20 -> 120 rpm in 0.5 s
	static constexpr uint32_t jog_timeout_ms{500U};		// [ms] no JOG this long: jog stops

	PumpController(const PumpCalculator& calculator, MotorInterface& motor);

	CalcStatus set_settings(float diameter_mm, float flow_ml_min);
	CommandStatus start();
	void stop();
	CommandStatus jog();
	void stop_jog();					// stops only a jog, a normal run goes on
	bool update(uint32_t now_ms);		// call often; true = the jog stopped by itself
	PumpState state() const;

private:
	void ramp_jog(uint32_t now_ms);		// makes the jog faster step by step

	const PumpCalculator& calculator_;
	MotorInterface& motor_;
	PumpState state_{PumpState::idle};
	bool settings_valid_{false};
	TimerSettings timer_{};
	bool jog_refreshed_{false};		// a JOG came since the last update()
	uint32_t last_jog_ms_{0U};		// [ms] time of the last JOG seen by update()
	float jog_now_rpm_{0.0F};		// [rpm] jog speed now (ramp)
	bool ramp_clock_set_{false};	// last_ramp_ms_ is valid
	uint32_t last_ramp_ms_{0U};		// [ms] time of the last ramp step
};
#endif // SYRINGE_PUMP_B__PUMP_CONTROLLER_HPP_
















