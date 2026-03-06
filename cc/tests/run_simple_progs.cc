#include <gtest/gtest.h>
#include "cc_helper.h"

TEST(SimpleProgs, Simplest)
{
    EXPECT_EQ("", compile_and_run(R"(
int main(void)
{
    return 2 + 1;
}
)", 3));
}

TEST(SimpleProgs, MultipleAdds)
{
    EXPECT_EQ("", compile_and_run(R"(
int main(void)
{
    return 2 + 1 + 4 + 5;
}
)", 12));
}

TEST(SimpleProgs, FloatAdds)
{
    EXPECT_EQ("", compile_and_run(R"(
int main(void)
{
    return 1.0 + 3.0;
}
)", 4));
}

TEST(SimpleProgs, PromotePartial)
{
    EXPECT_EQ("", compile_and_run(R"(
int main(void)
{
    return 2.3 + 3;
}
)", 5));
}

#if 0
TEST(SimpleProgs, FunctionCall)
{
    EXPECT_EQ("Hello, world\n", compile_and_run(R"(
#include <stdio.h>

int main(void)
{
    puts("Hello, world\n");
    return 0;
}
)"));
}

#endif
