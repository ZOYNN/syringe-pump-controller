// ================================================================
//  stepper_driver.hpp
//  Drives the A4988 stepper driver: enable, step rate.
//  One direction only: DIR (PB3) is always the push level.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__STEPPER_DRIVER_HPP_
#define SYRINGE_PUMP_B__STEPPER_DRIVER_HPP_

// ======================== INCLUDES ========================

#include "main.h"
#include "motor_interface.hpp"

// ========================= CLASS ==========================

class StepperDriver : public MotorInterface
{
public:
	explicit StepperDriver(TIM_HandleTypeDef& timer);

	void set_rate(const TimerSettings& settings) override;
	void start() override;
	void stop() override;
	bool is_running() const override;

private:
	TIM_HandleTypeDef& timer_;		// timer that makes the STEP pulses (TIM3)
	bool running_{false};

};

#endif // SYRINGE_PUMP_B__STEPPER_DRIVER_HPP_
