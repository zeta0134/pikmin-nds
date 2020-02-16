#include <nds.h>
#include <cstdio>

#include "tests/testing_framework.h"
#include "wide_console.h"

void init_test_console() {
	InitWideConsole();
	std::printf("\x1b[39m");
}


bool tests_exist_at_all() {
	return true;
}

void run_all_tests() {
	TEST(tests_exist_at_all);
}