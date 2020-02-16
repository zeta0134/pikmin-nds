#include "tests/testing_framework.h"
#include "tests/vram.h"
#include "render/vram_configuration.h"

bool partial_pass_lcdc_flips_between_two_banks() {
    const auto partial_0 = vram_configuration_for_pass(0, false);
    const auto partial_1 = vram_configuration_for_pass(1, false);
    const auto partial_2 = vram_configuration_for_pass(2, false);
    const auto partial_3 = vram_configuration_for_pass(3, false);
    
    bool even_banks_match = partial_0.lcdc_address == partial_2.lcdc_address;
    bool odd_banks_match = partial_1.lcdc_address == partial_3.lcdc_address;
    bool sequential_banks_dont_match = partial_0.lcdc_address != partial_1.lcdc_address;

    return even_banks_match && odd_banks_match && sequential_banks_dont_match;
}

bool final_pass_lcdc_differs_from_both_partial_passes() {
    const auto partial_0 = vram_configuration_for_pass(0, false);
    const auto partial_1 = vram_configuration_for_pass(1, false);
    const auto final_0 = vram_configuration_for_pass(0, true);
    const auto final_1 = vram_configuration_for_pass(1, true);

    bool final_even_bank_differs = partial_0.lcdc_address != final_0.lcdc_address;
    bool final_odd_bank_differs = partial_1.lcdc_address != final_1.lcdc_address;

    return final_even_bank_differs && final_odd_bank_differs;
}

bool address_is_mapped_as_lcd(u16* address, VramConfiguration config) {
	if (address == VRAM_A && config.bank_a_mode == VRAM_A_LCD) { return true; }
	if (address == VRAM_B && config.bank_b_mode == VRAM_B_LCD) { return true; }
	if (address == VRAM_C && config.bank_c_mode == VRAM_C_LCD) { return true; }
	if (address == VRAM_D && config.bank_d_mode == VRAM_D_LCD) { return true; }
	return false;
}

bool address_is_mapped_as_texture_slot_0(u16* address, VramConfiguration config) {
	if (address == VRAM_A && config.bank_a_mode == VRAM_A_TEXTURE_SLOT0) { return true; }
	if (address == VRAM_B && config.bank_b_mode == VRAM_B_TEXTURE_SLOT0) { return true; }
	if (address == VRAM_C && config.bank_c_mode == VRAM_C_TEXTURE_SLOT0) { return true; }
	if (address == VRAM_D && config.bank_d_mode == VRAM_D_TEXTURE_SLOT0) { return true; }
	return false;
}

bool lcd_address_is_actually_mapped_lcd() {
	for (unsigned int p = 0; p < 9; p++) {
		const auto partial = vram_configuration_for_pass(p, false);
		const auto final = vram_configuration_for_pass(p, true);
		if (!(address_is_mapped_as_lcd(partial.lcdc_address, partial))) {
			return false;
		}
		if (!(address_is_mapped_as_lcd(final.lcdc_address, final))) {
			return false;
		}
	}	
	return true;
}

bool previous_partial_capture_is_currently_texture_slot_0() {
	for (unsigned int p = 1; p < 9; p++) {
		const auto previous = vram_configuration_for_pass(p - 1, false);
		const auto current_partial = vram_configuration_for_pass(p, false);
		const auto current_final = vram_configuration_for_pass(p, true);
		if (!(address_is_mapped_as_texture_slot_0(previous.lcdc_address, current_partial))) {
			return false;
		}
		if (!(address_is_mapped_as_texture_slot_0(previous.lcdc_address, current_final))) {
			return false;
		}
	}
	return true;
}

void run_vram_tests() {
	TEST(partial_pass_lcdc_flips_between_two_banks);
	TEST(final_pass_lcdc_differs_from_both_partial_passes);
	TEST(lcd_address_is_actually_mapped_lcd);
	TEST(previous_partial_capture_is_currently_texture_slot_0);
}

