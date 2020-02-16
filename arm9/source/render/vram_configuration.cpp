#include "render/vram_configuration.h"

static const VramConfiguration MultipassPartialEven {
	VRAM_A_LCD,
	VRAM_B_TEXTURE_SLOT0,
	VRAM_C_TEXTURE_SLOT2,
	VRAM_D_MAIN_BG_0x06000000,
	VRAM_A
};

static const VramConfiguration MultipassPartialOdd {
	VRAM_A_TEXTURE_SLOT0,
	VRAM_B_LCD,
	VRAM_C_TEXTURE_SLOT2,
	VRAM_D_MAIN_BG_0x06000000,
	VRAM_B
};

static const VramConfiguration MultipassFinalEven {
	VRAM_A_LCD,            // Note: not used
	VRAM_B_TEXTURE_SLOT0,
	VRAM_C_TEXTURE_SLOT2,
	VRAM_D_LCD,
	VRAM_D
};

static const VramConfiguration MultipassFinalOdd {
	VRAM_A_TEXTURE_SLOT0,            
	VRAM_B_LCD,            // Note: not used
	VRAM_C_TEXTURE_SLOT2,
	VRAM_D_LCD,
	VRAM_D
};

VramConfiguration vram_configuration_for_pass(unsigned int pass_number, bool final_pass) {
	if (final_pass) {
		if (pass_number % 2 == 0) {
			return MultipassFinalEven;
		} else {
			return MultipassFinalOdd;
		}
	} else {
		if (pass_number % 2 == 0) {
			return MultipassPartialEven;
		} else {
			return MultipassPartialOdd;
		}
	}
}