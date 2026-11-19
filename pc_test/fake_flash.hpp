// ================================================================
//  fake_flash.hpp
//  FlashInterface in RAM for the PC tests. Acts like the STM32F103:
//  erase = 0xFF, a word can be written only when it is erased.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

#ifndef SYRINGE_PUMP_B__FAKE_FLASH_HPP_
#define SYRINGE_PUMP_B__FAKE_FLASH_HPP_

// ======================== INCLUDES ========================

#include <cstdint>
#include <vector>

#include "flash_interface.hpp"
#include "settings_store.hpp"

// ========================= CLASS ==========================

class FakeFlash : public FlashInterface
{
public:
	static constexpr uint32_t base{0x0800F800U};
	static constexpr uint32_t size{SettingsStore::page_size * SettingsStore::page_count};

	bool erase_page(const uint32_t page_address) override
	{
		if (fail)
		{
			return false;
		}
		const uint32_t first{(page_address - base) / 4U};
		for (uint32_t i{0U}; i < (SettingsStore::page_size / 4U); ++i)
		{
			words[first + i] = 0xFFFFFFFFU;
		}
		++erase_count;
		return true;
	}

	bool program_word(const uint32_t address, const uint32_t value) override
	{
		uint32_t& word{words[(address - base) / 4U]};
		if (fail || (word != 0xFFFFFFFFU))
		{
			return false;		// STM32F103: writing on a not erased word is an error
		}
		word = value;
		++write_count;
		return true;
	}

	uint32_t read_word(const uint32_t address) const override
	{
		return words[(address - base) / 4U];
	}

	std::vector<uint32_t> words = std::vector<uint32_t>(size / 4U, 0xFFFFFFFFU);
	bool fail{false};
	int erase_count{0};
	int write_count{0};
};

#endif // SYRINGE_PUMP_B__FAKE_FLASH_HPP_
