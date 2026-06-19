#include "test.h"

static int intentional_failure(void)
{
    TEST_ASSERT_QUIET(0);
    return EXIT_SUCCESS;
}

int main(void)
{
    const int failure_status = intentional_failure();

    TEST_ASSERT(failure_status != 0);
    TEST_ASSERT_EQ_INT(EXIT_FAILURE, failure_status);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, EXIT_SUCCESS);
    puts("test_assertions: ok");
    return EXIT_SUCCESS;
}
