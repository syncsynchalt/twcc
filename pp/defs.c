#include "defs.h"

#include <die.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "pp_toker.h"

// ReSharper disable CppDFANullDereference

static char **parse_args(const char *name, const char *args);

void defines_add(const defines *defs, const char *name, const char *args, const char *replace)
{
    def *d = calloc(1, sizeof(*d));
    d->name = strdup(name);
    d->args = parse_args(name, args);
    d->replace = replace ? strdup(replace) : NULL;

    hashmap_add(defs->h, name, d);
}

int defines_remove(const defines *defs, const char *name)
{
    const def *d = hashmap_delete(defs->h, name);
    if (d) {
        free(d->name);
        int i;
        for (i = 0; d->args && d->args[i]; i++) {
            free(d->args[i]);
        }
        free(d->replace);
        free((void *) d);
        return 1;
    }
    return 0;
}

static void skip_parens(token_state *ts)
{
    int parens_count = 1;
    while (!TOKEN_STATE_DONE(ts) && parens_count > 0) {
        const token t = get_token(ts);
        if (t.type == '(') {
            parens_count++;
        }
        if (t.type == ')') {
            parens_count--;
        }
    }
}

static char **parse_args(const char *name, const char *args)
{
    token_state ts;
    char **result = NULL;
    int comma_counter = 0;
    size_t n = 0;

    if (!args) {
        return NULL;
    }

    set_token_string(&ts, args);
    const size_t len = strlen(args);
    for (;;) {
        const token t = get_token(&ts);
        if (!len || t.type == TOK_WS) {
            // ignore
        } else if (t.type == '(') {
            // skip past parenthesized argument in args list
            skip_parens(&ts);
        } else if (t.type == ',') {
            if (comma_counter % 2 != 1) {
                die("#define %s args extra comma in %s", name, args);
            }
            comma_counter++;
        } else if (t.type == TOK_ID || IS_KEYWORD(t.type) || t.type == TOK_ELLIPSIS) {
            if (comma_counter % 2 != 0) {
                die("#define %s args missing comma in %s", name, args);
            }
            comma_counter++;
            if (n % 5 == 0) {
                result = realloc(result, sizeof(*result) * (n + 6));
            }
            result[n++] = strdup(t.tok);
        } else {
            die("#define %s(%s) bad arg \"%s\"", name, args, t.tok);
        }
        if (TOKEN_STATE_DONE(&ts)) {
            break;
        }
    }
    if (result && n) {
        result[n] = NULL;
    }
    return result;
}

defines *defines_init(void)
{
    defines *defs;

    defs = calloc(1, sizeof *defs);
    defs->h = hashmap_init(1024);

    // todo - implement
    defines_add(defs, "__FILE__", NULL, "\"__filename__\"");
    defines_add(defs, "__LINE__", NULL, "0");

    return defs;
}

def *defines_get(const defines *defs, const char *name)
{
    if (defs) {
        def *d = hashmap_get(defs->h, name);
        if (d && !d->ignored) {
            return d;
        }
    }
    return NULL;
}

void defines_destroy(defines *defs)
{
    hashmap_iter_state iter = {};
    hashmap_entry *e;

    while ((e = hashmap_iter(defs->h, &iter))) {
        def *d = e->data;
        free(d->name);
        int i;
        for (i = 0; d->args && d->args[i]; i++) {
            free(d->args[i]);
        }
        free(d->replace);
        free(d);
    }
    free(defs);
}

void clear_ignore_list(ignore_list *l)
{
    int i;
    for (i = 0; i < l->count; i++) {
        l->ignored[i]->ignored--;
    }
    free(l->ignored);
    l->ignored = NULL;
    l->count = 0;
}

void add_to_ignore_list(ignore_list *l, def *d)
{
    l->ignored = realloc(l->ignored, sizeof (*l->ignored) * (l->count + 1));
    l->ignored[l->count] = d;
    l->ignored[l->count++]->ignored++;
}
