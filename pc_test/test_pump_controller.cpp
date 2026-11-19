// ================================================================
//  test_pump_controller.cpp
//  Tests PumpController on the PC, with a fake motor.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <iostream>

#include "pump_controller.hpp"

// ======================= PARAMETERS =======================

constexpr float test_lead_mm{1.25F};		// [mm/turn]
constexpr float test_ratio{10.0F / 21.0F};	// belt ratio
constexpr float test_diameter_mm{19.2F};	// [mm]
constexpr float test_flow_ml_min{1.0F};		// [Ml/min]
constexpr float faster_flow_ml_min{2.0F};	// [mL/min]

// ======================== FAKE MOTOR ======================

class FakeMotor : public MotorInterface
{
public:
	void set_direction(Direction new_direction) override
	{
		direction = new_direction;
		++direction_writes;
	}

	void set_rate(const TimerSettings& settings) override
	{
		last_settings = settings;
	}

	void start() override
	{
		running = true;
	}

	void stop() override
	{
		running = false;
	}

	bool is_running() const override
	{
		return running;
	}

	bool running{false};
	TimerSettings last_settings{};
	Direction direction{Direction::forward};
	int direction_writes{0};		// how often the DIR pin was written
};

// ========================== HELPERS ==========================

bool same_timer(const TimerSettings& a, const TimerSettings& b)
{
	return (a.prescaler == b.prescaler) && (a.auto_reload == b.auto_reload);
}

// ========================== MAIN ==========================

int main()
{
	const PumpCalculator calculator{test_lead_mm, test_ratio, true};
	FakeMotor motor{};
	PumpController pump{calculator, motor};

	// 1. start without settings is refused
	assert(pump.state() == PumpState::idle);
	assert(pump.start() == CommandStatus::no_settings);
	assert(!motor.is_running());

	// 2. bad settings are refused
	assert(pump.set_settings(0.0F, test_flow_ml_min) == CalcStatus::bad_diameter);
	assert(pump.start() == CommandStatus::no_settings);

	// 3. good settings, then start: the motor gets the right rate
	assert(pump.set_settings(test_diameter_mm, test_flow_ml_min) == CalcStatus::ok);
	assert(pump.start() == CommandStatus::ok);
	assert(pump.state() == PumpState::running);
	assert(motor.is_running());
	const TimerSettings slow{calculator.timer_settings(calculator.step_hz(test_diameter_mm, test_flow_ml_min))};
	assert(same_timer(motor.last_settings, slow));

	// 4. new flow while running: new rate at once
	assert(pump.set_settings(test_diameter_mm, faster_flow_ml_min) == CalcStatus::ok);
	const TimerSettings fast{calculator.timer_settings(calculator.step_hz(test_diameter_mm, faster_flow_ml_min))};
	assert(same_timer(motor.last_settings, fast));

	// 5. bad flow while running: refused, the motor keeps going
	assert(pump.set_settings(test_diameter_mm, 0.0F) == CalcStatus::bad_flow);
	assert(motor.is_running());
	assert(same_timer(motor.last_settings, fast));

	// 7. stop
	pump.stop();
	assert(pump.state() == PumpState::idle);
	assert(!motor.is_running());

	// 8. direction: refused while running, OK when idle
	pump.stop();
	assert(pump.direction() == Direction::forward);
	assert(pump.set_direction(Direction::reverse) == CommandStatus::ok);		// idle
	assert(pump.direction() == Direction::reverse);
	assert(motor.direction == Direction::reverse);
	assert(pump.start() == CommandStatus::ok);
	assert(pump.set_direction(Direction::forward) == CommandStatus::busy);		// running
	assert(pump.direction() == Direction::reverse);
	assert(motor.direction == Direction::reverse);
	assert(motor.is_running());
	pump.stop();
	assert(pump.set_direction(Direction::forward) == CommandStatus::ok);		// idle
	assert(motor.direction == Direction::forward);
	assert(pump.start() == CommandStatus::ok);
	assert(pump.state() == PumpState::running);

	// 9. jog: refused while running; works without settings, in the chosen direction, fixed speed
	assert(pump.jog() == CommandStatus::busy);
	pump.stop();
	assert(pump.set_direction(Direction::reverse) == CommandStatus::ok);
	FakeMotor jog_motor{};
	PumpController jog_pump{calculator, jog_motor};		// no settings at all
	assert(jog_pump.set_direction(Direction::reverse) == CommandStatus::ok);
	const int writes_before_jog{jog_motor.direction_writes};
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(jog_pump.state() == PumpState::jogging);
	assert(jog_motor.is_running());
	assert(jog_motor.direction == Direction::reverse);				// GERİ chosen: jog goes back
	assert(jog_motor.direction_writes == writes_before_jog);		// jog does not touch the DIR pin
	const TimerSettings jog_start_rate{calculator.timer_settings(calculator.step_hz_for_rpm(PumpController::jog_start_rpm))};
	assert(same_timer(jog_motor.last_settings, jog_start_rate));		// starts slow (ramp)
	assert(jog_pump.direction() == Direction::reverse);			// the chosen direction is kept

	// 10. while jogging: RUN and direction are refused
	assert(jog_pump.start() == CommandStatus::busy);
	assert(jog_pump.set_direction(Direction::forward) == CommandStatus::busy);

	// 11. STP ends the jog: motor stops, direction stays the chosen one
	jog_pump.stop();
	assert(jog_pump.state() == PumpState::idle);
	assert(!jog_motor.is_running());
	assert(jog_motor.direction == Direction::reverse);

	// 11b. stop_jog: ends a jog, but never a normal run
	assert(jog_pump.jog() == CommandStatus::ok);
	jog_pump.stop_jog();
	assert(jog_pump.state() == PumpState::idle);
	assert(!jog_motor.is_running());
	assert(pump.state() == PumpState::idle);
	assert(pump.start() == CommandStatus::ok);
	pump.stop_jog();
	assert(pump.state() == PumpState::running);
	assert(motor.is_running());
	pump.stop();

	// 12. timeout: no JOG for jog_timeout_ms -> stops by itself
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(!jog_pump.update(1000U));							// sees the JOG
	assert(!jog_pump.update(1000U + PumpController::jog_timeout_ms - 1U));
	assert(jog_pump.jog() == CommandStatus::ok);				// JOG again (button held)
	assert(!jog_pump.update(1400U));
	assert(!jog_pump.update(1400U + PumpController::jog_timeout_ms - 1U));
	assert(jog_pump.state() == PumpState::jogging);
	assert(jog_pump.update(1400U + PumpController::jog_timeout_ms));		// lost: stop
	assert(jog_pump.state() == PumpState::idle);
	assert(!jog_motor.is_running());
	assert(jog_motor.direction == Direction::reverse);
	assert(!jog_pump.update(5000U));							// idle: nothing to do

	// 13. ramp: jog starts at jog_start_rpm, +jog_ramp_step_rpm every jog_ramp_interval_ms, up to jog_rpm
	const TimerSettings full_rate{calculator.timer_settings(calculator.step_hz_for_rpm(PumpController::jog_rpm))};
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(same_timer(jog_motor.last_settings, jog_start_rate));
	uint32_t t{10000U};
	assert(!jog_pump.update(t));												// starts the ramp clock
	assert(!jog_pump.update(t + PumpController::jog_ramp_interval_ms - 1U));
	assert(same_timer(jog_motor.last_settings, jog_start_rate));				// too early: no change
	float expected_rpm{PumpController::jog_start_rpm};
	while (expected_rpm < PumpController::jog_rpm)
	{
		t += PumpController::jog_ramp_interval_ms;
		assert(jog_pump.jog() == CommandStatus::ok);							// button held
		assert(!jog_pump.update(t));
		expected_rpm += PumpController::jog_ramp_step_rpm;
		if (expected_rpm > PumpController::jog_rpm)
		{
			expected_rpm = PumpController::jog_rpm;
		}
		assert(same_timer(jog_motor.last_settings, calculator.timer_settings(calculator.step_hz_for_rpm(expected_rpm))));
	}
	assert(same_timer(jog_motor.last_settings, full_rate));
	t += PumpController::jog_ramp_interval_ms;
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(!jog_pump.update(t));
	assert(same_timer(jog_motor.last_settings, full_rate));					// stays at full speed
	assert(jog_motor.direction == Direction::reverse);
	jog_pump.stop_jog();
	assert(jog_motor.direction == Direction::reverse);

	// 14. a new jog starts slow again
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(same_timer(jog_motor.last_settings, jog_start_rate));
	jog_pump.stop_jog();

	// 15. İLERİ chosen: jog goes forward, DIR pin not written by jog / stop
	assert(jog_pump.set_direction(Direction::forward) == CommandStatus::ok);
	const int writes_forward{jog_motor.direction_writes};
	assert(jog_pump.jog() == CommandStatus::ok);
	assert(jog_motor.direction == Direction::forward);
	jog_pump.stop_jog();
	assert(jog_motor.direction == Direction::forward);
	assert(jog_motor.direction_writes == writes_forward);


	std::cout << "pump controller: ok\n";
	return 0;
}

