#ifndef TESTING_FRAMEWORK_H
#define TESTING_FRAMEWORK_H

#include <nds.h>
#include <cstdio>

#define TEST(test) do { \
    bool result = test(); \
    std::printf("%s %s\n", result ? "ok" : "not ok", #test); \
    nocashMessage((result ? "ok " : "not ok ")); \
    nocashMessage(#test); \
    nocashMessage("\n"); \
  } while(false);

#define HEADER(message) do { \
  	std::printf("\n=== %s ===\n", message); \
  	nocashMessage("\n=== "); \
  	nocashMessage(message); \
  	nocashMessage(" ===\n"); \
  } while(false);

void init_test_console();
void run_all_tests();

#endif