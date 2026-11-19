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

SavedSettings make(const float diameter_mm, const float flow, const Direction direction,
				   const FlowUnit unit = FlowUnit::ml_per_min)
{
	SavedSettings settings{};
	settings.has_values = true;
	settings.diameter_mm = diameter_mm;
	settings.flow = flow;
	settings.flow_unit = unit;
	settings.direction = direction;
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
	assert(store.save(make(19.2F, 1.5F, Direction::forward)));
	SavedSettings loaded{load_after_power_cut(flash, found)};
	assert(found);
	assert(loaded.has_values);
	assert(near(loaded.diameter_mm, 19.2F));
	assert(near(loaded.flow, 1.5F));
	assert(loaded.direction == Direction::forward);

	// 3. the same values again: nothing is written
	const int writes{flash.write_count};
	assert(store.save(make(19.2F, 1.5F, Direction::forward)));
	assert(flash.write_count == writes);

	// 4. only the last values count
	assert(store.save(make(20.5F, 2.0F, Direction::reverse)));
	loaded = load_after_power_cut(flash, found);
	assert(near(loaded.diameter_mm, 20.5F));
	assert(near(loaded.flow, 2.0F));
	assert(loaded.direction == Direction::reverse);

	// 5. direction only (no values yet)
	FakeFlash flash2{};
	SettingsStore store2{flash2, FakeFlash::base};
	SavedSettings direction_only{};
	direction_only.direction = Direction::reverse;
	assert(store2.save(direction_only));
	loaded = load_after_power_cut(flash2, found);
	assert(found);
	assert(!loaded.has_values);
	assert(loaded.direction == Direction::reverse);

	// 6. many saves: the ring goes round both pages many times,
	//    a page is erased once per 64 saves, the last value always comes back
	FakeFlash flash3{};
	SettingsStore store3{flash3, FakeFlash::base};
	constexpr int many_saves{1000};
	for (int i{1}; i <= many_saves; ++i)
	{
		assert(store3.save(make(10.0F + (static_cast<float>(i) / 100.0F), 1.0F, Direction::forward)));
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
		assert(store3.save(make(30.0F, (i % 2 == 0) ? 1.0F : 2.0F, Direction::forward)));
	}
	loaded = load_after_power_cut(flash3, found);
	assert(near(loaded.flow, 2.0F));		// i = 69999 (odd) was the last

	// 8. a record cut by a power loss (only 2 of 4 words written):
	//    the old values come back, the next save jumps over the broken place
	FakeFlash flash4{};
	SettingsStore store4{flash4, FakeFlash::base};
	assert(store4.save(make(19.2F, 1.5F, Direction::forward)));
	flash4.words[4] = 0x53500001U;		// slot 1, word 0
	flash4.words[5] = 1234U;				// slot 1, word 1 (word 2 and 3 never came)
	loaded = load_after_power_cut(flash4, found);
	assert(found);
	assert(near(loaded.diameter_mm, 19.2F));
	SettingsStore store4b{flash4, FakeFlash::base};
	assert(store4b.save(make(25.0F, 3.0F, Direction::reverse)));
	loaded = load_after_power_cut(flash4, found);
	assert(near(loaded.diameter_mm, 25.0F));
	assert(loaded.direction == Direction::reverse);

	// 10. flow unit is kept; only a new unit is a change (written)
	FakeFlash flash6{};
	SettingsStore store6{flash6, FakeFlash::base};
	assert(store6.save(make(19.2F, 1.5F, Direction::reverse, FlowUnit::ml_per_hour)));
	loaded = load_after_power_cut(flash6, found);
	assert(found);
	assert(near(loaded.flow, 1.5F));
	assert(loaded.flow_unit == FlowUnit::ml_per_hour);
	assert(loaded.direction == Direction::reverse);
	assert(store6.save(make(19.2F, 9999.99F, Direction::forward, FlowUnit::ul_per_min)));		// biggest input
	loaded = load_after_power_cut(flash6, found);
	assert(near(loaded.flow, 9999.99F));
	assert(loaded.flow_unit == FlowUnit::ul_per_min);
	assert(loaded.direction == Direction::forward);
	const int writes_before{flash6.write_count};
	assert(store6.save(make(19.2F, 9999.99F, Direction::forward, FlowUnit::ul_per_min)));
	assert(flash6.write_count == writes_before);		// same: nothing written
	assert(store6.save(make(19.2F, 9999.99F, Direction::forward, FlowUnit::ml_per_min)));
	assert(flash6.write_count > writes_before);			// only the unit changed: written

	// 11. a record from the old firmware (no unit bits) is read as mL/min
	FakeFlash flash7{};
	flash7.words[0] = 0x53500000U;					// magic + sequence 0
	flash7.words[1] = 1920U;						// 19.20 mm
	flash7.words[2] = 150U | 0x80000000U;			// 1.50 mL/min, reverse
	flash7.words[3] = ~(flash7.words[0] ^ flash7.words[1] ^ flash7.words[2]);
	loaded = load_after_power_cut(flash7, found);
	assert(found);
	assert(near(loaded.flow, 1.5F));
	assert(loaded.flow_unit == FlowUnit::ml_per_min);
	assert(loaded.direction == Direction::reverse);

	// 9. flash error: save says false
	FakeFlash flash5{};
	SettingsStore store5{flash5, FakeFlash::base};
	flash5.fail = true;
	assert(!store5.save(make(19.2F, 1.5F, Direction::forward)));

	std::cout << "settings store: ok\n";
	return 0;
}
