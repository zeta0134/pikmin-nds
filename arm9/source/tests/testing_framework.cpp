#include <nds.h>
#include <cstdio>

#include "tests/testing_framework.h"
#include "wide_console.h"

void init_test_console() {
	InitWideConsole();
	std::printf("\x1b[39m");
}

#define TEST(test) do { \
    bool result = test(); \
    std::printf("%s %s\n", result ? "ok" : "not ok", #test); \
    nocashMessage((result ? "ok" : "not ok")); \
    nocashMessage(#test); \
  } while(false);


bool tests_exist_at_all() {
	return true;
}

void run_all_tests() {
	TEST(tests_exist_at_all);
}