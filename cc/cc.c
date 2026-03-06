#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "parse.h"
#include "common.h"
#include "die.h"
#include "asm.h"

static char *infile = NULL;
static char *outfile = "a.out";

void usage(int argc, char **argv)
{
    die("Usage: %s infile [-o outfile] [-I include_path]", argv[0]);
}

static FILE *preprocess_file(const char *filename, char **include_paths)
{
    char ppout[256] = "/tmp/twpp.tmp.XXXXXX";
    mkstemp(ppout);
    char cmd[1024];

    snprintf(cmd, sizeof(cmd), "twpp -o %s", ppout);
    size_t i, l;
    for (i = 0; include_paths[i]; i++) {
        l = strlen(cmd);
        snprintf(cmd + l, sizeof(cmd) - l, " -I \"%s\"", include_paths[i]);
    }
    l = strlen(cmd);
    snprintf(cmd + l, sizeof(cmd) - l, " %s", filename);

    if (system(cmd) != 0) {
        die("Error running '%s', exiting", cmd);
    }
    FILE *f = fopen(ppout, "r");
    if (!f) {
        die("Can't open %s: %s", ppout, strerror(errno));
    }
    unlink(ppout);
    return f;
}

static void as_out(const ast_node *ast, char *file, size_t file_sz)
{
    snprintf(file, file_sz, "/tmp/twcc.tmp.XXXXXX");
    mkstemp(file);

    FILE *f = fopen(file, "w");
    if (!f) {
        die("Can't open %s: %s", file, strerror(errno));
    }
    read_ast(ast, f);
    fclose(f);
}

static void run_as(const char *in, const char *out)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "as \"%s\" -o \"%s\"", in, out);
    const int ret = system(cmd);
    if (ret) {
        die("Error running assembler");
    }
}

int main(const int argc, char **argv)
{
    int arg;
    size_t num_include_paths = 0;
    char **include_paths = calloc(1, sizeof(*include_paths) * 1);
    int pp_only = 0;
    int as_only = 0;

    while ((arg = getopt(argc, argv, "ESo:I:")) != -1) {
        if (arg == '?' || arg == ':') {
            usage(argc, argv);
        }
        if (arg == 'E') {
            pp_only = 1;
        }
        if (arg == 'S') {
            as_only = 1;
        }
        if (arg == 'o') {
            outfile = optarg;
        }
        if (arg == 'I') {
            if (num_include_paths % 5 == 0) {
                include_paths = realloc(include_paths, sizeof(*include_paths) * (num_include_paths + 6));
            }
            include_paths[num_include_paths++] = strdup(optarg);
        }
    }
    include_paths[num_include_paths] = NULL;
    if (optind >= argc) {
        usage(argc, argv);
    }

    infile = argv[optind++];
    if (!outfile && optind < argc) {
        outfile = argv[optind++];
    }
    if (optind < argc) {
        usage(argc, argv);
    }

    FILE *in = preprocess_file(infile, include_paths);
    if (pp_only) {
        char buf[64];
        while (fgets(buf, sizeof(buf), in)) {
            fputs(buf, stdout);
        }
        exit(0);
    }

    const ast_node *ast = parse_ast(infile, in);
    char as_file[64];
    as_out(ast, as_file, sizeof(as_file));
    if (as_only) {
        char buf[64];
        FILE *f = fopen(as_file, "r");
        while (fgets(buf, sizeof(buf), f)) {
            fputs(buf, stdout);
        }
        unlink(as_file);
        exit(0);
    }
    run_as(as_file, outfile);
    unlink(as_file);

    int i;
    for (i = 0; i < num_include_paths; i++) {
        free((void *)include_paths[i]);
    }
    free(include_paths);
}
