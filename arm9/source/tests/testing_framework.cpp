#include "wide_console.h"

#include "tests/testing_framework.h"
#include "tests/vram.h"

void init_test_console() {
	InitWideConsole();
	std::printf("\x1b[39m");
}


bool tests_exist_at_all() {
	return true;
}

void run_all_tests() {
    HEADER("Extreme Basics");
	TEST(tests_exist_at_all);
    HEADER("Vram Configuration");
    run_vram_tests();
}