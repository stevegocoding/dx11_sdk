#include "tests.h"

void start_scene_test(int *argc, wchar_t **argv)
{
    testing::InitGoogleTest(argc, argv);
    RUN_ALL_TESTS();
}