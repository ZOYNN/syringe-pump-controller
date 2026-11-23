// ================================================================
//  stepper_driver.cpp
//  Drives the A4988 stepper driver: enable, step rate.
//  One direction only: DIR (PB3) is always the push level.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include "stepper_driver.hpp"

// ======================= PARAMETERS =======================

constexpr uint32_t step_channel{TIM_CHANNEL_1};		// STEP pin PB4 = TIM3 channel 1
constexpr GPIO_PinState push_level{GPIO_PIN_RESET};	// DIR PB3 LOW = plunger goes forward (Papa's tests, 2026-11-23)

// ======================== CONSTRUCTOR =====================

StepperDriver::StepperDriver(TIM_HandleTypeDef& timer) : timer_{timer}
{

}

// ======================== METHODS =========================

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
	HAL_GPIO_WritePin(DRV_DIR_GPIO_Port, DRV_DIR_Pin, push_level);		// before the first step (main.c also sets LOW)
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











