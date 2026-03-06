#include <gtest/gtest.h>
#include "cc_helper.h"

TEST(TypedefTest, RegisterTypeName)
{
    const auto ast = run_grammar(R"(
typedef int foo;
int main(int argc, char **argv)
{
    int a = 1;
    foo b = 2;
}
)");

    const auto def = ast->list[0];
    const auto fn = ast->list[1];
    const auto statements = fn->list[0];
    const auto decl1 = statements->list[0];
    const auto decl2 = statements->list[1];

    EXPECT_EQ(R"(
(
  (nt:decl)
  (
    typedef
    int
    NULL
  )
  (
    (nt:list)
    [
      (
        =
        ID:foo
        NULL
      )
    ]
  )
)
)", PrintAst(def));


    EXPECT_EQ(R"(
(
  (nt:decl)
  int
  (
    (nt:list)
    [
      (
        =
        ID:a
        INT:1
      )
    ]
  )
)
)", PrintAst(decl1));

    EXPECT_EQ(R"(
(
  (nt:decl)
  TYPENAME:foo
  (
    (nt:list)
    [
      (
        =
        ID:b
        INT:2
      )
    ]
  )
)
)", PrintAst(decl2));
}
