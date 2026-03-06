#include <gtest/gtest.h>
#include "cc_helper.h"
#include <iostream>
#include <fstream>
extern "C" {
#include "parse.h"
}

ast_node *run_grammar(const std::string &input)
{
    const auto infile = write_file(input);
    FileDeleter fd(infile);
    return run_grammar_on_file(infile);
}

ast_node *run_grammar_on_file(const std::string &filename)
{
    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "Failed to open file " << filename << std::endl;
        return nullptr;
    }
    ast_node *ast = parse_ast(filename.c_str(), f);
    fclose(f);
    return ast;
}

static void print_file_to_stderr(const std::string &filename)
{
    const auto in = std::ifstream(filename, std::ios::in);
    std::cerr << in.rdbuf() << std::endl;
}

std::string compile_and_run(const std::string &program, int expect_retcode)
{
    char t_file[256] = "/tmp/test.XXXXXX";
    mkstemp(t_file);
    const auto t = std::string(t_file);
    std::ofstream file(t + ".c");
    file << program;
    file.close();

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "PATH=../../pp:$PATH ../twcc -o %s.o %s.c 2> %s.err", t_file, t_file, t_file);
    int rc = system(cmd);
    if (rc) {
        std::cerr << "Failed to run command " << cmd << std::endl;
        print_file_to_stderr(t + ".err");
        return "cc died";
    }

    snprintf(cmd, sizeof(cmd), "cc -o %s.runme %s.o 2> %s.err", t_file, t_file, t_file);
    rc = system(cmd);
    if (rc) {
        std::cerr << "Failed to run command " << cmd << std::endl;
        print_file_to_stderr(t + ".err");
        return "linker died";
    }

    snprintf(cmd, sizeof(cmd), "%s.runme &> %s.out", t_file, t_file);
    rc = system(cmd);
    EXPECT_TRUE(WIFEXITED(rc));
    EXPECT_EQ(expect_retcode, WEXITSTATUS(rc));

    std::ifstream in(t + ".out");
    std::string result;
    std::string line;
    while (std::getline(in, line)) {
        result += line + "\n";
    }
    unlink((t + ".c").c_str());
    unlink((t + ".o").c_str());
    unlink((t + ".err").c_str());
    unlink((t + ".runme").c_str());
    unlink((t + ".out").c_str());
    return result;
}
