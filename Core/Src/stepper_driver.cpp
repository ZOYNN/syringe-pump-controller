// ================================================================
//  stepper_driver.cpp
//  Drives the A4988 stepper driver: enable, direction, step rate.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "stepper_driver.hpp"

// ======================= PARAMETERS =======================

constexpr uint32_t step_channel{TIM_CHANNEL_1};		// STEP pin PB4 = TIM3 channel 1

// ======================== CONSTRUCTOR =====================

StepperDriver::StepperDriver(TIM_HandleTypeDef& timer) : timer_{timer}
{

}

// ======================== METHODS =========================

void StepperDriver::set_direction(const Direction direction)
{
	const GPIO_PinState level{(direction == Direction::reverse) ? GPIO_PIN_SET : GPIO_PIN_RESET};
	HAL_GPIO_WritePin(DRV_DIR_GPIO_Port, DRV_DIR_Pin, level);
}

void StepperDriver::set_rate(const TimerSettings& settings)
{
	const uint32_t half_period{(static_cast<uint32_t>(settings.auto_reload) + 1U) / 2U};		// 50% duty
	__HAL_TIM_SET_PRESCALER(&timer_, settings.prescaler);
	__HAL_TIM_SET_AUTORELOAD(&timer_, settings.auto_reload);
	__HAL_TIM_SET_COMPARE(&timer_, step_channel, half_period);

	if (!running_)
	{
		timer_.Instance->EGR = TIM_EGR_UG;		// load the new values now, not after the next period
	}
}

void  StepperDriver::start()
{
	if (running_)
	{
		return;
	}
	HAL_GPIO_WritePin(DRV_ENABLE_GPIO_Port, DRV_ENABLE_Pin, GPIO_PIN_RESET);		// active low: driver on
	HAL_TIM_PWM_Start(&timer_, step_channel);
	running_ = true;
}

void StepperDriver::stop()
{
	HAL_TIM_PWM_Stop(&timer_, step_channel);
	HAL_GPIO_WritePin(DRV_ENABLE_GPIO_Port, DRV_ENABLE_Pin, GPIO_PIN_SET);			// active low: driver off
	running_ = false;
}

bool StepperDriver::is_running() const
{
	return running_;
}











