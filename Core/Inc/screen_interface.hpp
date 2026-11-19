// ================================================================
//  screen_interface.hpp
//  What the program needs from a screen: show a text in an object.
//  The real Nextion screen and the fake screen of the PC test
//  both follow this interface.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

#ifndef SYRINGE_PUMP_B__SCREEN_INTERFACE_HPP_
#define SYRINGE_PUMP_B__SCREEN_INTERFACE_HPP_

// ========================= CLASS ==========================

class ScreenInterface
{
public:
	// e.g. show("t_status", "HAZIR") -> t_status.txt="HAZIR"
	virtual void show(const char* object, const char* text) = 0;

protected:
	~ScreenInterface() = default;		// no delete through this type (no heap)
};

#endif // SYRINGE_PUMP_B__SCREEN_INTERFACE_HPP_
