// ================================================================
//  settings_store.hpp
//  Keeps the last diameter, flow, flow unit and direction in the flash,
//  so they are back after a power cut (M7).
//  Uses 2 flash pages as a ring of 16-byte records. A new record
//  goes to the next free place; a page is erased only when the
//  ring comes to it again (1 erase per 64 saves).
//  No hardware here (FlashInterface), so it can be tested on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

#ifndef SYRINGE_PUMP_B__SETTINGS_STORE_HPP_
#define SYRINGE_PUMP_B__SETTINGS_STORE_HPP_

// ======================== INCLUDES ========================

#include <cstdint>

#include "flash_interface.hpp"
#include "motor_interface.hpp"
#include "pump_calculator.hpp"

// ========================= STRUCTS ========================

struct SavedSettings
{
	bool has_values{false};		// diameter and flow are known
	float diameter_mm{0.0F};
	float flow{0.0F};			// the number on the screen, in flow_unit
	FlowUnit flow_unit{FlowUnit::ml_per_min};
	Direction direction{Direction::forward};
};

// ========================= CLASS ==========================

class SettingsStore
{
public:
	static constexpr uint32_t page_size{1024U};			// [bytes] STM32F103C8
	static constexpr uint32_t page_count{2U};
	static constexpr uint32_t record_words{4U};
	static constexpr uint32_t record_size{record_words * 4U};		// [bytes]
	static constexpr uint32_t records_per_page{page_size / record_size};	// 64
	static constexpr uint32_t record_count{records_per_page * page_count};	// 128

	SettingsStore(FlashInterface& flash, uint32_t base_address);

	bool load(SavedSettings& settings);			// false: nothing saved yet
	bool save(const SavedSettings& settings);	// false: flash error; same values: nothing written

private:
	struct Record
	{
		uint32_t words[record_words]{};
	};

	uint32_t slot_address(uint32_t slot) const;
	Record read_record(uint32_t slot) const;
	bool slot_is_free(uint32_t slot) const;
	bool page_is_free(uint32_t page) const;
	bool find_newest(uint32_t& slot, uint16_t& sequence) const;

	static bool is_valid(const Record& record);
	static uint32_t checksum(const Record& record);
	static Record make_record(const SavedSettings& settings, uint16_t sequence);
	static SavedSettings from_record(const Record& record);
	static bool same(const SavedSettings& a, const SavedSettings& b);

	FlashInterface& flash_;
	uint32_t base_address_;
	bool has_last_{false};		// last_ = what is in the flash now
	SavedSettings last_{};
};

#endif // SYRINGE_PUMP_B__SETTINGS_STORE_HPP_
