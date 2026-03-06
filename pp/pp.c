#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "common.h"
#include "defs.h"
#include "preprocessor.h"
#include "die.h"

static char *infile = NULL;
static char *outfile = NULL;

void usage(int argc, char **argv)
{
    die("Usage: %s infile [[-o] outfile] [-I include_path]", argv[0]);
}

const char *find_include_path(int argc, char **argv)
{
    char *include_path = NULL;
    char *trypath = NULL;
    char buf[256];

    if (argv[0][0] == '/') {
        trypath = strdup(argv[0]);
    } else {
        getcwd(buf, sizeof(buf));
        trypath = malloc(strlen(buf) + 1 + strlen(argv[0]));
        sprintf(trypath, "%s/%s", buf, argv[0]);
    }

    while (*trypath) {
        include_path = malloc(strlen(trypath) + strlen("/include") + 1);
        sprintf(include_path, "%s/include", trypath);
        if (access(include_path, X_OK) == 0) {
            return include_path;
        }
        free(include_path);
        if (strrchr(trypath, '/')) {
            strrchr(trypath, '/')[0] = '\0';
        } else {
            break;
        }
    }

    fprintf(stderr, "Unable to find include path using all roots of path %s", dirname(argv[0]));
    exit(1);
}

int main(const int argc, char **argv)
{
    const char *base_include = find_include_path(argc, argv);

    size_t num_include_paths = 0;
    const char **include_paths = calloc(sizeof(char *), 6);
    include_paths[num_include_paths++] = base_include;

    int arg;
    while ((arg = getopt(argc, argv, "o:I:")) != -1) {
        if (arg == '?' || arg == ':') {
            usage(argc, argv);
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

    FILE *in = fopen(infile, "r");
    if (!in) {
        die("Can't open %s for reading: %s", infile, strerror(errno));
    }
    FILE *out = outfile ? fopen(outfile, "w") : stdout;
    if (!out) {
        die("Can't open %s for writing: %s", outfile, strerror(errno));
    }
    output = out;

    defines *defs = defines_init();

    parse_state state = {0};
    state.defs = defs;
    state.include_paths = include_paths;
    process_file(infile, in, out, &state);
    fclose(out);
    fclose(in);

    int i;
    for (i = 0; i < num_include_paths; i++) {
        free((void *)include_paths[i]);
    }
    free(include_paths);
}
