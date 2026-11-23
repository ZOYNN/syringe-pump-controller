// ================================================================
//  test_settings_store.cpp
//  Tests SettingsStore on the PC with a fake flash.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-16
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <cmath>
#include <iostream>

#include "fake_flash.hpp"
#include "settings_store.hpp"

// ========================== HELPERS ==========================

bool near(const float a, const float b)
{
	return std::fabs(a - b) < 0.001F;
}

SavedSettings make(const float diameter_mm, const float flow,
				   const FlowUnit unit = FlowUnit::ml_per_min)
{
	SavedSettings settings{};
	settings.has_values = true;
	settings.diameter_mm = diameter_mm;
	settings.flow = flow;
	settings.flow_unit = unit;
	return settings;
}

// what a new SettingsStore finds after a power cut
SavedSettings load_after_power_cut(FakeFlash& flash, bool& found)
{
	SettingsStore store{flash, FakeFlash::base};
	SavedSettings settings{};
	found = store.load(settings);
	return settings;
}

// ========================== MAIN ==========================

int main()
{
	bool found{false};

	// 1. empty flash: nothing to load
	FakeFlash flash{};
	load_after_power_cut(flash, found);
	assert(!found);

	// 2. save, power cut, load: the same values
	SettingsStore store{flash, FakeFlash::base};
	assert(store.save(make(19.2F, 1.5F)));
	SavedSettings loaded{load_after_power_cut(flash, found)};
	assert(found);
	assert(loaded.has_values);
	assert(near(loaded.diameter_mm, 19.2F));
	assert(near(loaded.flow, 1.5F));

	// 3. the same values again: nothing is written
	const int writes{flash.write_count};
	assert(store.save(make(19.2F, 1.5F)));
	assert(flash.write_count == writes);

	// 4. only the last values count
	assert(store.save(make(20.5F, 2.0F)));
	loaded = load_after_power_cut(flash, found);
	assert(near(loaded.diameter_mm, 20.5F));
	assert(near(loaded.flow, 2.0F));

	// 5. unit only (no values yet)
	FakeFlash flash2{};
	SettingsStore store2{flash2, FakeFlash::base};
	SavedSettings unit_only{};
	unit_only.flow_unit = FlowUnit::ml_per_hour;
	assert(store2.save(unit_only));
	loaded = load_after_power_cut(flash2, found);
	assert(found);
	assert(!loaded.has_values);
	assert(loaded.flow_unit == FlowUnit::ml_per_hour);
	assert((flash2.words[2] & 0x80000000U) == 0U);		// bit 31 (old direction bit) is not written

	// 6. many saves: the ring goes round both pages many times,
	//    a page is erased once per 64 saves, the last value always comes back
	FakeFlash flash3{};
	SettingsStore store3{flash3, FakeFlash::base};
	constexpr int many_saves{1000};
	for (int i{1}; i <= many_saves; ++i)
	{
		assert(store3.save(make(10.0F + (static_cast<float>(i) / 100.0F), 1.0F)));
		loaded = load_after_power_cut(flash3, found);
		assert(found);
		assert(near(loaded.diameter_mm, 10.0F + (static_cast<float>(i) / 100.0F)));
	}
	// the 2 pages are empty at the start (no erase); then 1 erase at each page start:
	// saves 129, 193, ... 961 -> 14 erases
	assert(flash3.erase_count == 14);

	// 7. sequence number 65535 -> 0 still finds the newest
	for (int i{0}; i < 70000; ++i)
	{
		assert(store3.save(make(30.0F, (i % 2 == 0) ? 1.0F : 2.0F)));
	}
	loaded = load_after_power_cut(flash3, found);
	assert(near(loaded.flow, 2.0F));		// i = 69999 (odd) was the last

	// 8. a record cut by a power loss (only 2 of 4 words written):
	//    the old values come back, the next save jumps over the broken place
	FakeFlash flash4{};
	SettingsStore store4{flash4, FakeFlash::base};
	assert(store4.save(make(19.2F, 1.5F)));
	flash4.words[4] = 0x53500001U;		// slot 1, word 0
	flash4.words[5] = 1234U;				// slot 1, word 1 (word 2 and 3 never came)
	loaded = load_after_power_cut(flash4, found);
	assert(found);
	assert(near(loaded.diameter_mm, 19.2F));
	SettingsStore store4b{flash4, FakeFlash::base};
	assert(store4b.save(make(25.0F, 3.0F)));
	loaded = load_after_power_cut(flash4, found);
	assert(near(loaded.diameter_mm, 25.0F));

	// 10. flow unit is kept; only a new unit is a change (written)
	FakeFlash flash6{};
	SettingsStore store6{flash6, FakeFlash::base};
	assert(store6.save(make(19.2F, 1.5F, FlowUnit::ml_per_hour)));
	loaded = load_after_power_cut(flash6, found);
	assert(found);
	assert(near(loaded.flow, 1.5F));
	assert(loaded.flow_unit == FlowUnit::ml_per_hour);
	assert(store6.save(make(19.2F, 9999.99F, FlowUnit::ul_per_min)));		// biggest input
	loaded = load_after_power_cut(flash6, found);
	assert(near(loaded.flow, 9999.99F));
	assert(loaded.flow_unit == FlowUnit::ul_per_min);
	const int writes_before{flash6.write_count};
	assert(store6.save(make(19.2F, 9999.99F, FlowUnit::ul_per_min)));
	assert(flash6.write_count == writes_before);		// same: nothing written
	assert(store6.save(make(19.2F, 9999.99F, FlowUnit::ml_per_min)));
	assert(flash6.write_count > writes_before);			// only the unit changed: written

	// 11. a record from the old firmware (no unit bits, direction bit 31 = reverse):
	//     read as mL/min, the direction bit is ignored
	FakeFlash flash7{};
	flash7.words[0] = 0x53500000U;					// magic + sequence 0
	flash7.words[1] = 1920U;						// 19.20 mm
	flash7.words[2] = 150U | 0x80000000U;			// 1.50 mL/min, reverse
	flash7.words[3] = ~(flash7.words[0] ^ flash7.words[1] ^ flash7.words[2]);
	loaded = load_after_power_cut(flash7, found);
	assert(found);
	assert(near(loaded.flow, 1.5F));
	assert(near(loaded.diameter_mm, 19.2F));
	assert(loaded.flow_unit == FlowUnit::ml_per_min);
	SettingsStore store7{flash7, FakeFlash::base};
	SavedSettings old_values{};
	assert(store7.load(old_values));
	const int writes7{flash7.write_count};
	assert(store7.save(make(19.2F, 1.5F)));		// same values: no new record only because of bit 31
	assert(flash7.write_count == writes7);
	assert(store7.save(make(19.2F, 1.6F)));		// a new value: new record without bit 31
	assert((flash7.words[6] & 0x80000000U) == 0U);	// slot 1, word 2
	loaded = load_after_power_cut(flash7, found);
	assert(near(loaded.flow, 1.6F));

	// 9. flash error: save says false
	FakeFlash flash5{};
	SettingsStore store5{flash5, FakeFlash::base};
	flash5.fail = true;
	assert(!store5.save(make(19.2F, 1.5F)));

	std::cout << "settings store: ok\n";
	return 0;
}
