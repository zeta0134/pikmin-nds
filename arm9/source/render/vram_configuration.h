#ifndef TEST_VRAM_CONFIGURATION_H
#define TEST_VRAM_CONFIGURATION_H

#include <nds/ndstypes.h>
#include <nds/arm9/video.h>

typedef struct {
	VRAM_A_TYPE bank_a_mode;
	VRAM_B_TYPE bank_b_mode;
	VRAM_C_TYPE bank_c_mode;
	VRAM_D_TYPE bank_d_mode;
	u16* lcdc_address;
} VramConfiguration;

VramConfiguration vram_configuration_for_pass(unsigned int pass_number, bool final_pass);

#endif