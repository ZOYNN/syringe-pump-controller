// ================================================================
//  stm_flash.cpp
//  FlashInterface for the STM32F103 (HAL flash functions).
//  Note: while the flash is erased (about 20-40 ms) the CPU waits.
//  The STEP signal comes from the timer hardware, so the motor
//  is not affected; a UART byte can be lost in that moment.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

// ======================== INCLUDES ========================

#include "stm_flash.hpp"

#include "main.h"

// ======================== METHODS =========================

bool StmFlash::erase_page(const uint32_t page_address)
{
	FLASH_EraseInitTypeDef erase{};
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.PageAddress = page_address;
	erase.NbPages = 1U;
	uint32_t page_error{0U};		// HAL writes the bad page here (not used)

	HAL_FLASH_Unlock();
	const HAL_StatusTypeDef status{HAL_FLASHEx_Erase(&erase, &page_error)};
	HAL_FLASH_Lock();
	return status == HAL_OK;
}

bool StmFlash::program_word(const uint32_t address, const uint32_t value)
{
	HAL_FLASH_Unlock();
	const HAL_StatusTypeDef status{HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value)};
	HAL_FLASH_Lock();
	return status == HAL_OK;
}

uint32_t StmFlash::read_word(const uint32_t address) const
{
	return *reinterpret_cast<const volatile uint32_t*>(address);		// flash can be read like RAM
}
