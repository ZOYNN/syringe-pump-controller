// ================================================================
//  flash_interface.hpp
//  What SettingsStore needs from the flash memory.
//  The STM32 version is StmFlash; the PC tests use a fake flash.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

#ifndef SYRINGE_PUMP_B__FLASH_INTERFACE_HPP_
#define SYRINGE_PUMP_B__FLASH_INTERFACE_HPP_

// ======================== INCLUDES ========================

#include <cstdint>

// ========================= CLASS ==========================

class FlashInterface
{
public:
	virtual bool erase_page(uint32_t page_address) = 0;				// all bytes -> 0xFF
	virtual bool program_word(uint32_t address, uint32_t value) = 0;	// only on an erased word
	virtual uint32_t read_word(uint32_t address) const = 0;

protected:
	~FlashInterface() = default;		// no delete through this type (no heap)
};

#endif // SYRINGE_PUMP_B__FLASH_INTERFACE_HPP_
