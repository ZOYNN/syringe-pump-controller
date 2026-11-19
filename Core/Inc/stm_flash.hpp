// ================================================================
//  stm_flash.hpp
//  FlashInterface for the STM32F103 (HAL flash functions).
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

#ifndef SYRINGE_PUMP_B__STM_FLASH_HPP_
#define SYRINGE_PUMP_B__STM_FLASH_HPP_

// ======================== INCLUDES ========================

#include "flash_interface.hpp"

// ========================= CLASS ==========================

class StmFlash : public FlashInterface
{
public:
	bool erase_page(uint32_t page_address) override;
	bool program_word(uint32_t address, uint32_t value) override;
	uint32_t read_word(uint32_t address) const override;
};

#endif // SYRINGE_PUMP_B__STM_FLASH_HPP_
