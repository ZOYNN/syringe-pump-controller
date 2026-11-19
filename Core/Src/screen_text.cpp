// ================================================================
//  screen_text.cpp
//  The Turkish texts that the Blue Pill shows on the screen.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include "screen_text.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

// ==================== TURKISH LETTERS =====================
//  iso-8859-9 codes. A "\x.." code must be its own string piece:
//  "\xC7" "AP" is right, "\xC7AP" is wrong (A is read as a hex digit).
//  Ç = \xC7   Ğ = \xD0   İ = \xDD   Ö = \xD6   Ş = \xDE   Ü = \xDC   µ = \xB5

// ======================== FUNCTIONS =======================

const char* ScreenText::state_text(const PumpState state)
{
	switch (state)
	{
	case PumpState::idle:
		return "HAZIR";
	case PumpState::running:
		return "\xC7" "ALI" "\xDE" "IYOR";					// ÇALIŞIYOR
	case PumpState::jogging:
		return "\xDD" "LER" "\xDD" " S" "\xDC" "R" "\xDC" "L" "\xDC" "YOR";	// İLERİ SÜRÜLÜYOR
	}
	return "";
}

const char* ScreenText::direction_text(const Direction direction)
{
	return (direction == Direction::reverse)
		? "GER" "\xDD"										// GERİ
		: "\xDD" "LER" "\xDD";								// İLERİ
}

// dk = dakika (minute), sa = saat (hour)
const char* ScreenText::unit_text(const FlowUnit unit)
{
	switch (unit)
	{
	case FlowUnit::ml_per_min:
		return "mL/dk";
	case FlowUnit::ml_per_hour:
		return "mL/sa";
	case FlowUnit::ul_per_min:
		return "\xB5" "L/dk";								// µL/dk
	}
	return "";
}

const char* ScreenText::parse_error_text(const ParseStatus status)
{
	switch (status)
	{
	case ParseStatus::ok:
	case ParseStatus::empty:
		return "";
	case ParseStatus::unknown_command:
		return "B" "\xDD" "L" "\xDD" "NMEYEN KOMUT";		// BİLİNMEYEN KOMUT
	case ParseStatus::bad_value:
		return "HATALI DE" "\xD0" "ER";						// HATALI DEĞER
	}
	return "";
}

const char* ScreenText::calc_error_text(const CalcStatus status)
{
	switch (status)
	{
	case CalcStatus::ok:
		return "";
	case CalcStatus::bad_diameter:
		return "HATALI " "\xC7" "AP";						// HATALI ÇAP
	case CalcStatus::bad_volume:
		return "HATALI HAC" "\xDD" "M";						// HATALI HACİM
	case CalcStatus::bad_flow:
		return "HATALI AKI" "\xDE";							// HATALI AKIŞ
	case CalcStatus::bad_lead:
		return "HATALI V" "\xDD" "DA ADIMI";				// HATALI VİDA ADIMI
	case CalcStatus::step_rate_too_low:
		return "AKI" "\xDE" " " "\xC7" "OK D" "\xDC" "\xDE" "\xDC" "K";	// AKIŞ ÇOK DÜŞÜK
	case CalcStatus::step_rate_too_high:
		return "AKI" "\xDE" " " "\xC7" "OK Y" "\xDC" "KSEK";	// AKIŞ ÇOK YÜKSEK
	}
	return "";
}

const char* ScreenText::command_error_text(const CommandStatus status)
{
	switch (status)
	{
	case CommandStatus::ok:
		return "";
	case CommandStatus::no_settings:
		return "\xD6" "NCE " "\xC7" "AP VE AKI" "\xDE" " G" "\xDD" "R" "\xDD" "N";	// ÖNCE ÇAP VE AKIŞ GİRİN
	case CommandStatus::busy:
		return "\xD6" "NCE DURDURUN";						// ÖNCE DURDURUN
	}
	return "";
}

const char* ScreenText::save_error_text()
{
	return "KAYIT HATASI";
}

bool ScreenText::format_value(const float value, const std::size_t decimals,
							  char* out, const std::size_t size)
{
	if ((out == nullptr) || (decimals > max_decimals) || !(value >= 0.0F) || (value > max_value))
	{
		return false;		// !(value >= 0) is also true for NaN
	}

	uint32_t divider{1U};		// 1, 10, 100 or 1000
	for (std::size_t i{0U}; i < decimals; ++i)
	{
		divider *= 10U;
	}

	// 19.2 with 2 decimals -> 1920 -> whole 19, fraction 20
	const uint32_t scaled{static_cast<uint32_t>(std::lround(value * static_cast<float>(divider)))};
	const unsigned long whole{static_cast<unsigned long>(scaled / divider)};
	const unsigned long fraction{static_cast<unsigned long>(scaled % divider)};

	int written{0};
	if (decimals == 0U)
	{
		written = std::snprintf(out, size, "%lu", whole);
	}
	else
	{
		written = std::snprintf(out, size, "%lu.%0*lu", whole, static_cast<int>(decimals), fraction);
	}
	return (written > 0) && (static_cast<std::size_t>(written) < size);
}
