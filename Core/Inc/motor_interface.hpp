// ================================================================
//  motor_interface.hpp
//  What the pump logic needs from a motor. The real driver and
//  the fake motor of the PC test both follow this interface.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__MOTOR_INTERFACE_HPP_
#define SYRINGE_PUMP_B__MOTOR_INTERFACE_HPP_

// ======================== INCLUDES ========================

#include "pump_calculator.hpp"

// ========================= CLASS ==========================

class MotorInterface
{
public:
	virtual void set_rate(const TimerSettings& settings) = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual bool is_running() const = 0;

protected:
	~MotorInterface() = default;		// no delete through this type (no heap)
};
#endif // SYRINGE_PUMP_B__MOTOR_INTERFACE_HPP_
