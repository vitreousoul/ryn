#include <stdio.h>


#if RYN_TEST_csv

static void TestFunction(void)
{
    printf("Running test for ryn_csv... but not really.\n");
}

#elif RYN_TEST_prof

static void TestFunction(void)
{
    printf("Running test for ryn_prof... but not really.\n");
}

#else

static void TestFunction(void)
{
    printf("Error: No test function specified.\n");
}

#endif


int main(void)
{
    TestFunction();
}
