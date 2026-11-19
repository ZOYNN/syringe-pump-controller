// ================================================================
//  app.hpp
//  Entry points of the C++ application, called from main.c.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-12
// ================================================================

#ifndef SYRINGE_PUMP_B__APP_HPP_
#define SYRINGE_PUMP_B__APP_HPP_

// ========================= FUNCTIONS ======================

#ifdef __cplusplus
extern "C" {
#endif

void app_setup(void);		// called once, after the CubeMx init
void app_loop(void);		// called again and again in while (1)

#ifdef __cplusplus
}
#endif

#endif	// SYRINGE_PUMP_B__APP_HPP_
