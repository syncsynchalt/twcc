#include <gtest/gtest.h>
#include "pp_helper.h"
extern "C" {
#include "preprocessor.h"
}

TEST(ParseTest, EmptyFile)
{
    const auto output = run_parser(R"(
)");
    EXPECT_EQ("\n", output);
}

TEST(ParseTest, CommentReal)
{
    const auto contents = R"(/*
 * This comment contains tokens that the lexer will choke on if it
 * attempts to read this as a non-comment, such as the string that
 * follows: (the 'License').
 */
foo
)";

    const auto infile = write_file(contents);
    const auto outfile = write_file("");
    FileDeleter fd1(infile), fd2(outfile);
    FILE *fin = fopen(infile.c_str(), "r");
    FILE *fout = fopen(outfile.c_str(), "w");
    process_file(infile.c_str(), fin, fout, nullptr);
    fclose(fin);
    fclose(fout);
    EXPECT_EQ(" \nfoo\n", strip_line_hints(read_file(outfile)));
}

TEST(ParseTest, SolidCommentBlock)
{
    // error while parsing comments of continuous asterisks
    auto output = run_parser(R"(
/******************************************************************************
 *  Even number of asterisks.                                                 *
 ******************************************************************************/
)");
    EXPECT_EQ("\n \n", output);
    output = run_parser(R"(
/*****************************************************************************
 *  Odd number of asterisks.                                                 *
 *****************************************************************************/
)");
    EXPECT_EQ("\n \n", output);
}

TEST(ParseTest, TernaryShortCircuit)
{
    const auto output = run_parser(R"(
#define FOO
#if defined(FOO) ? 1 : AVOID_ME
 foo
#endif
#if defined(BAR) ? AVOID_ME : 0
 bar
#endif
)");
    EXPECT_EQ("\n foo\n", output);
}

TEST(ParseTest, LeadingSpace)
{
    const auto output = run_parser(std::string("\t") + R"(  #define FOO
    # if defined(FOO)
 bar
    # endif
)");
    EXPECT_EQ(" bar\n", output);
}

TEST(ParseTest, LeadingDirective)
{
    const auto output = run_parser("#define FOO 1\n#if FOO\n bar\n#endif\n");
    EXPECT_EQ(" bar\n", output);
}

TEST(ParseTest, NotEqual)
{
    const auto output = run_parser(R"(
#define FOO 1
#if FOO != 2
 foo
#else
 bar
#endif
)");
    EXPECT_EQ("\n foo\n", output);
}

TEST(ParseTest, DirectiveInComment)
{
    const auto output = run_parser(R"(
/*
 * This is a comment, with embedded directives.
# define FOO 123
 */
# define BAR 234
BAR
)");
    EXPECT_EQ("\n \n234\n", output);
}
