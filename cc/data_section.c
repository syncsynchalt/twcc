#include "data_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

char **lines;
size_t num_lines;

void add_to_data_section(const char *fmt, ...)
{
    char buf[64];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, sizeof(buf), fmt, ap);
    lines = realloc(lines, ++num_lines * sizeof(char *)); // NOLINT(*-suspicious-realloc-usage)
    lines[num_lines-1] = strdup(buf);
}

void print_data_section(FILE *out)
{
    fprintf(out, "\n");
    fprintf(out, ".data\n");
    size_t i;
    for (i = 0; i < num_lines; i++) {
        fprintf(out, "    %s\n", lines[i]);
    }
}
