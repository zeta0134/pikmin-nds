#ifndef TESTING_FRAMEWORK_H
#define TESTING_FRAMEWORK_H

#define TEST(test) do { \
    bool result = test(); \
    std::printf("%s %s\n", result ? "ok" : "not ok", #test); \
    nocashMessage((result ? "ok " : "not ok ")); \
    nocashMessage(#test); \
    nocashMessage("\n"); \
  } while(false);

void init_test_console();
void run_all_tests();

#endif