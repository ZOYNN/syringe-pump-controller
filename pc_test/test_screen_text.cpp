// ================================================================
//  test_screen_text.cpp
//  Tests ScreenText and NextionCommand on the PC.
//  Author : Bahadir Sevimlier
//  Date   : 2026-11-13
// ================================================================

// ======================== INCLUDES ========================

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "nextion_command.hpp"
#include "screen_text.hpp"

// ========================== HELPERS ==========================

// iso-8859-9 (screen) -> UTF-8 (this file), only for the Turkish capital letters and µ
std::string to_utf8(const char* text)
{
	std::string result{};
	for (const char* p{text}; *p != '\0'; ++p)
	{
		switch (static_cast<unsigned char>(*p))
		{
		case 0xC7U: result += "Ç"; break;
		case 0xD0U: result += "Ğ"; break;
		case 0xDDU: result += "İ"; break;
		case 0xD6U: result += "Ö"; break;
		case 0xDEU: result += "Ş"; break;
		case 0xDCU: result += "Ü"; break;
		case 0xB5U: result += "µ"; break;
		default:    result += *p; break;
		}
	}
	return result;
}

bool same(const char* screen_text, const char* utf8_text)
{
	return to_utf8(screen_text) == utf8_text;
}

bool formats_to(const float value, const std::size_t decimals, const char* expected)
{
	char out[16]{};
	return ScreenText::format_value(value, decimals, out, sizeof(out)) && (std::strcmp(out, expected) == 0);
}

// ========================== MAIN ==========================

int main()
{
	// 1. states and units
	assert(same(ScreenText::state_text(PumpState::idle), "HAZIR"));
	assert(same(ScreenText::state_text(PumpState::running), "ÇALIŞIYOR"));
	assert(same(ScreenText::state_text(PumpState::jogging), "İLERİ SÜRÜLÜYOR"));
	assert(same(ScreenText::unit_text(FlowUnit::ml_per_min), "mL/dk"));
	assert(same(ScreenText::unit_text(FlowUnit::ml_per_hour), "mL/sa"));
	assert(same(ScreenText::unit_text(FlowUnit::ul_per_min), "µL/dk"));

	// 2. error texts ("" for ok)
	assert(same(ScreenText::parse_error_text(ParseStatus::ok), ""));
	assert(same(ScreenText::parse_error_text(ParseStatus::empty), ""));
	assert(same(ScreenText::parse_error_text(ParseStatus::unknown_command), "BİLİNMEYEN KOMUT"));
	assert(same(ScreenText::parse_error_text(ParseStatus::bad_value), "HATALI DEĞER"));
	assert(same(ScreenText::calc_error_text(CalcStatus::ok), ""));
	assert(same(ScreenText::calc_error_text(CalcStatus::bad_diameter), "HATALI ÇAP"));
	assert(same(ScreenText::calc_error_text(CalcStatus::bad_volume), "HATALI HACİM"));
	assert(same(ScreenText::calc_error_text(CalcStatus::bad_flow), "HATALI AKIŞ"));
	assert(same(ScreenText::calc_error_text(CalcStatus::bad_lead), "HATALI VİDA ADIMI"));
	assert(same(ScreenText::calc_error_text(CalcStatus::step_rate_too_low), "AKIŞ ÇOK DÜŞÜK"));
	assert(same(ScreenText::calc_error_text(CalcStatus::step_rate_too_high), "AKIŞ ÇOK YÜKSEK"));
	assert(same(ScreenText::command_error_text(CommandStatus::ok), ""));
	assert(same(ScreenText::command_error_text(CommandStatus::no_settings), "ÖNCE ÇAP VE AKIŞ GİRİN"));
	assert(same(ScreenText::command_error_text(CommandStatus::busy), "ÖNCE DURDURUN"));
	assert(same(ScreenText::save_error_text(), "KAYIT HATASI"));

	// 3. numbers
	assert(formats_to(19.2F, 2U, "19.20"));
	assert(formats_to(0.01F, 2U, "0.01"));
	assert(formats_to(2.0F, 0U, "2"));
	assert(formats_to(0.0F, 2U, "0.00"));
	assert(formats_to(9999.99F, 2U, "9999.99"));
	char small[4]{};
	assert(!ScreenText::format_value(19.2F, 2U, small, sizeof(small)));		// "19.20" needs 6 chars
	assert(!ScreenText::format_value(-1.0F, 2U, small, sizeof(small)));
	assert(!ScreenText::format_value(1.0F, 3U, small, sizeof(small)));

	// 4. a full screen command: t_status.txt="ÇALIŞIYOR" + FF FF FF
	char command[48]{};
	const std::size_t length{NextionCommand::make_text("t_status", ScreenText::state_text(PumpState::running),
													   command, sizeof(command))};
	const char expected[]{"t_status.txt=\"\xC7" "ALI" "\xDE" "IYOR\"\xFF\xFF\xFF"};
	assert(length == sizeof(expected) - 1U);
	assert(std::memcmp(command, expected, length) == 0);

	std::cout << "screen text: ok\n";
	return 0;
}
