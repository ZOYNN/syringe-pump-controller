// ================================================================
//  settings_store.cpp
//  Keeps the last diameter, flow and flow unit in the flash.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================
//
//  One record = 4 words (16 bytes):
//    word 0: magic (high 16 bits) + sequence number (low 16 bits)
//    word 1: diameter in 1/100 mm      (19.20 mm -> 1920)
//    word 2: flow in 1/100 of its unit (1.50 -> 150), bits 0-28
//            bits 29-30 = flow unit (0 mL/min, 1 mL/h, 2 µL/min)
//            bit 31 = old direction bit (until 2026-11-23): not written, ignored when read
//            Records from before the units (2026-11-17) have bits 29-30 = 0 = mL/min.
//    word 3: checksum of words 0-2
//  Erased flash is 0xFFFFFFFF: a free place has all 4 words = 0xFFFFFFFF.
//  The newest record is the valid one with the highest sequence number.
//  A record that was cut by a power loss has a bad checksum: it is skipped.

// ======================== INCLUDES ========================

#include "settings_store.hpp"

#include <cmath>

// ======================= PARAMETERS =======================

constexpr uint32_t magic{0x5350U};				// "SP" = syringe pump
constexpr uint32_t erased_word{0xFFFFFFFFU};
constexpr uint32_t unit_shift{29U};				// word 2, bits 29-30
constexpr uint32_t unit_mask{0x3U};
constexpr uint32_t value_mask{0x1FFFFFFFU};		// word 2, bits 0-28
constexpr float centi{100.0F};					// values are kept with 2 decimals

// ======================== HELPERS =========================

namespace
{
uint32_t to_centi(const float value)
{
	return static_cast<uint32_t>(std::lround(value * centi));
}

// unknown code (cannot be written by this program): mL/min
FlowUnit unit_from_code(const uint32_t code)
{
	if (code == static_cast<uint32_t>(FlowUnit::ml_per_hour))
	{
		return FlowUnit::ml_per_hour;
	}
	if (code == static_cast<uint32_t>(FlowUnit::ul_per_min))
	{
		return FlowUnit::ul_per_min;
	}
	return FlowUnit::ml_per_min;
}
} // namespace

// ======================== CONSTRUCTOR =====================

SettingsStore::SettingsStore(FlashInterface& flash, const uint32_t base_address)
	: flash_{flash}, base_address_{base_address}
{

}

// ======================== METHODS =========================

bool SettingsStore::load(SavedSettings& settings)
{
	uint32_t slot{0U};
	uint16_t sequence{0U};
	if (!find_newest(slot, sequence))
	{
		return false;
	}

	settings = from_record(read_record(slot));
	last_ = settings;
	has_last_ = true;
	return true;
}

bool SettingsStore::save(const SavedSettings& settings)
{
	if (has_last_ && same(settings, last_))
	{
		return true;		// nothing changed: no flash write (saves flash life)
	}

	// next place: after the newest record (or the start when the flash is empty)
	uint32_t slot{0U};
	uint16_t sequence{0U};
	if (find_newest(slot, sequence))
	{
		slot = (slot + 1U) % record_count;
		++sequence;
	}

	// at the start of a page: erase it (it has the oldest records);
	// inside a page: jump over places that are not free (cut records)
	for (uint32_t tries{0U}; tries < record_count; ++tries)
	{
		if ((slot % records_per_page) == 0U)
		{
			const uint32_t page{slot / records_per_page};
			if (!page_is_free(page) && !flash_.erase_page(slot_address(slot)))
			{
				return false;
			}
			break;
		}
		if (slot_is_free(slot))
		{
			break;
		}
		slot = (slot + 1U) % record_count;
	}

	const Record record{make_record(settings, sequence)};
	const uint32_t address{slot_address(slot)};
	for (uint32_t i{0U}; i < record_words; ++i)
	{
		if (!flash_.program_word(address + (i * 4U), record.words[i]))
		{
			return false;
		}
	}

	// read back and check
	const Record check{read_record(slot)};
	for (uint32_t i{0U}; i < record_words; ++i)
	{
		if (check.words[i] != record.words[i])
		{
			return false;
		}
	}

	last_ = from_record(record);		// same rounding as after a load
	has_last_ = true;
	return true;
}

uint32_t SettingsStore::slot_address(const uint32_t slot) const
{
	return base_address_ + (slot * record_size);
}

SettingsStore::Record SettingsStore::read_record(const uint32_t slot) const
{
	Record record{};
	const uint32_t address{slot_address(slot)};
	for (uint32_t i{0U}; i < record_words; ++i)
	{
		record.words[i] = flash_.read_word(address + (i * 4U));
	}
	return record;
}

bool SettingsStore::slot_is_free(const uint32_t slot) const
{
	const Record record{read_record(slot)};
	for (uint32_t i{0U}; i < record_words; ++i)
	{
		if (record.words[i] != erased_word)
		{
			return false;
		}
	}
	return true;
}

bool SettingsStore::page_is_free(const uint32_t page) const
{
	for (uint32_t i{0U}; i < records_per_page; ++i)
	{
		if (!slot_is_free((page * records_per_page) + i))
		{
			return false;
		}
	}
	return true;
}

// newest = valid record with the highest sequence number
// (the difference as int16 also works when the number goes 65535 -> 0)
bool SettingsStore::find_newest(uint32_t& slot, uint16_t& sequence) const
{
	bool found{false};
	for (uint32_t i{0U}; i < record_count; ++i)
	{
		const Record record{read_record(i)};
		if (!is_valid(record))
		{
			continue;
		}
		const uint16_t number{static_cast<uint16_t>(record.words[0] & 0xFFFFU)};
		if (!found || (static_cast<int16_t>(static_cast<uint16_t>(number - sequence)) > 0))
		{
			slot = i;
			sequence = number;
			found = true;
		}
	}
	return found;
}

bool SettingsStore::is_valid(const Record& record)
{
	return ((record.words[0] >> 16U) == magic) && (record.words[3] == checksum(record));
}

uint32_t SettingsStore::checksum(const Record& record)
{
	return ~(record.words[0] ^ record.words[1] ^ record.words[2]);
}

SettingsStore::Record SettingsStore::make_record(const SavedSettings& settings, const uint16_t sequence)
{
	Record record{};
	record.words[0] = (magic << 16U) | sequence;
	record.words[1] = settings.has_values ? to_centi(settings.diameter_mm) : 0U;
	record.words[2] = settings.has_values ? (to_centi(settings.flow) & value_mask) : 0U;
	record.words[2] |= (static_cast<uint32_t>(settings.flow_unit) & unit_mask) << unit_shift;
	record.words[3] = checksum(record);
	return record;
}

SavedSettings SettingsStore::from_record(const Record& record)
{
	SavedSettings settings{};
	const uint32_t diameter{record.words[1]};
	const uint32_t flow{record.words[2] & value_mask};
	settings.has_values = (diameter > 0U) && (flow > 0U);
	settings.diameter_mm = static_cast<float>(diameter) / centi;
	settings.flow = static_cast<float>(flow) / centi;
	settings.flow_unit = unit_from_code((record.words[2] >> unit_shift) & unit_mask);
	return settings;
}

// compared with 2 decimals, like they are kept in the flash
bool SettingsStore::same(const SavedSettings& a, const SavedSettings& b)
{
	if ((a.has_values != b.has_values) || (a.flow_unit != b.flow_unit))
	{
		return false;
	}
	if (!a.has_values)
	{
		return true;
	}
	return (to_centi(a.diameter_mm) == to_centi(b.diameter_mm)) && (to_centi(a.flow) == to_centi(b.flow));
}
