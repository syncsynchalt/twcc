#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ast.h"
#include "common.h"
#include "die.h"
#include "lex.h"
#include "str.h"
#include "pp_ast.h"
#include "pp_macro.h"

static void process_directive(token_state *ts, parse_state *state);
static void process_tokens(token_state *ts, parse_state *state);

// ReSharper disable CppParameterMayBeConstPtrOrRef

void process_file(const char *filename, FILE *in, FILE *out, parse_state *existing_state)
{
    current_file = filename;
    current_lineno = 0;
    fprintf(out, "# %d \"%s\"\n", current_lineno+1, current_file);

    parse_state ps = {0};
    ps.defs = existing_state ? existing_state->defs : NULL;
    ps.include_paths = existing_state ? existing_state->include_paths : NULL;
    ps.out = out;
    ps.once_filenames =
            existing_state && existing_state->once_filenames ? existing_state->once_filenames : hashmap_init(32);
    ps.current_filename = filename;

    token_state *ts = &ps.ts;
    set_token_file(ts, in, filename);

    while (!feof(in)) {
        if (TOKEN_STATE_DIRECTIVE(ts)) {
            process_directive(ts, &ps);
        } else {
            if (output_active) {
                process_tokens(ts, &ps);
            } else {
                skip_line(ts);
            }
        }
    }
    current_line = NULL;
    if (ferror(in)) {
        die("reading input file");
    }

    if (ps.top_if) {
        die("Missing #endif in %s", filename);
    }
}

static int resolve_include_path(char *filename, const size_t len, const char **include_paths)
{
    char buf[256];
    int i;
    if (filename[0] == '/' && access(filename, F_OK) == 0) {
        return 1;
    }
    for (i = 0; include_paths[i]; i++) {
        snprintf(buf, sizeof(buf), "%s/%s", include_paths[i], filename);
        if (access(buf, F_OK) == 0) {
            snprintf(filename, len, "%s", buf);
            return 1;
        }
    }
    return 0;
}

static int resolve_condition(const char *condition, parse_state *state)
{
    const ast_node *node = string_to_ast(condition, state->defs);
    const ast_result pp_result = resolve_ast(node);
    switch (pp_result.type) {
        case AST_RESULT_TYPE_INT:
        default:
            return !!pp_result.ival;
        case AST_RESULT_TYPE_FLT:
            return !!pp_result.fval;
        case AST_RESULT_TYPE_STR:
            return !!strlen(pp_result.sval);
    }
}

static void add_if_frame(parse_state *state)
{
    struct if_frame *new_if_frame = calloc(1, sizeof *new_if_frame);
    new_if_frame->masked = state->top_if && !state->top_if->is_true;
    new_if_frame->next = state->top_if;
    state->top_if = new_if_frame;
}

static void mark_true(parse_state *state, const int truth)
{
    state->top_if->is_true = 0;
    output_active = 0;
    if (!state->top_if->truth_found) {
        state->top_if->is_true = truth;
        output_active = truth;
        state->top_if->truth_found = truth;
    }
}

static void handle_ifdef(const char *rest, parse_state *state, const int invert)
{
    if (state->top_if->masked) {
        return;
    }
    token_state ts;
    set_token_string(&ts, rest);
    const token t = get_token(&ts);
    const int defined = defines_get(state->defs, t.tok) ? 1 : 0;
    mark_true(state, invert ? !defined : defined);
}

static void handle_if(const char *rest, parse_state *state)
{
    if (state->top_if->masked) {
        return;
    }
    const char *condition = rest;
    mark_true(state, resolve_condition(condition, state));
}

static void handle_else(parse_state *state)
{
    if (!state->top_if) {
        die("#else outside of #if");
    }
    if (state->top_if->masked) {
        return;
    }
    mark_true(state, 1);
}

static void handle_endif(parse_state *state)
{
    if (!state->top_if) {
        die("#endif unmatched");
    }
    struct if_frame *i = state->top_if;
    state->top_if = state->top_if->next;
    output_active = !state->top_if || (state->top_if->is_true && !state->top_if->masked);
    free(i);
}

static void handle_include(const char *rest, parse_state *state)
{
    token_state ts;
    set_token_string(&ts, rest);
    char filename[256] = {};

    token t = get_token(&ts);
    if (t.type == TOK_STR) {
        if (!decode_str(t.tok, filename, sizeof(filename))) {
            die("Unable to decode filename %s", t.tok);
        }
    } else if (t.type == '<') {
        for (;;) {
            t = get_token(&ts);
            if (t.type == EOF) {
                die("#include unterminated filename %s", rest);
            }
            if (t.type == '>') {
                break;
            }
            snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename), "%s", t.tok);
        }
    } else {
        die("#include unrecognized filename %s", rest);
    }

    if (!resolve_include_path(filename, sizeof(filename), state->include_paths)) {
        die("file %s not found in include paths", filename);
    }
    const char *existing_file = current_file;
    const int existing_line = current_lineno;
    FILE *in = fopen(filename, "r");
    if (!in) {
        die("Unable to open file %s: %s", filename, strerror(errno));
    }
    process_file(filename, in, state->out, state);
    fclose(in);
    current_file = existing_file;
    current_lineno = existing_line;
    fprintf(state->out, "# %d \"%s\"\n", current_lineno + 1, current_file);
}

int span_parens(const char *p)
{
    int i = 0;
    int parens_count = 0;
    for (i = 0; p[i] != '\0'; i++) {
        if (p[i] == '(') {
            parens_count++;
        }
        if (p[i] == ')') {
            parens_count--;
            if (parens_count == 0) {
                return i;
            }
        }
    }
    return i;
}

static void handle_define(const char *rest, parse_state *state)
{
    char *name = NULL;
    char *args = NULL;

    token_state ts;
    set_token_string(&ts, rest);
    const token t = get_token(&ts);
    if (t.type != TOK_ID && !IS_KEYWORD(t.type)) {
        die("#define must be an identifier: %s", rest);
    }
    name = strdup(t.tok);

    // todo xxx don't use ind directly
    const char *p = rest + ts.ind;
    if (*p == '(') {
        const char *q = p + span_parens(p);
        if (*q != ')') {
            die("#define missing closing parens: %s", rest);
        }
        args = malloc(q - p + 1);
        strncpy(args, p + 1, q - p - 1);
        args[q-p-1] = '\0';
        p = q + 1;
    }
    p += strspn(p, " \t");

    defines_add(state->defs, name, args, p);
    free(name);
    free(args);
}

static void handle_undef(const char *rest, const parse_state *state)
{
    token_state ts = {};
    set_token_string(&ts, rest);
    const token t = get_token(&ts);
    if (t.type != TOK_ID && !IS_KEYWORD(t.type)) {
        die("#undef must be an identifier: %s", rest);
    }
    defines_remove(state->defs, t.tok);
}

static void handle_line_directive(const char *rest, const parse_state *state)
{
    /*
     * directive will be in the form of '#line nnn "filename"'
     * which matches our output format for line hinting, so copy it as-is.
     */
    fprintf(state->out, "# %s\n", rest);
}

#define STARTS_WITH(str, chk) (strncmp(str, chk, sizeof(chk)-1) == 0)

static void handle_pragma(const char *rest, const parse_state *state)
{
    char buf[16];
    const char *end = rest + strcspn(rest, WHITESPACE);
    snprintf(buf, sizeof(buf), "%.*s", (int) (end - rest), rest);
    if (strcmp(buf, "once") == 0) {
        if (hashmap_get(state->once_filenames, state->current_filename)) {
            fseek(state->ts.f, 0, SEEK_END);
        } else {
            char *key = strdup(state->current_filename);
            hashmap_add(state->once_filenames, key, key);
        }
    } else if (STARTS_WITH(rest, "clang diagnostic ")) {
        // ignore
    } else if (STARTS_WITH(rest, "GCC poison ")) {
        // ignore
    } else if (STARTS_WITH(rest, "GCC error \"Already found AvailabilityVersions")) {
        // ignore
    } else {
        fprintf(stderr, "Warning: unrecognized pragma %s\n", rest);
    }
}


/**
 * Process the directive given in ts->line
 */
static void process_directive(token_state *ts, parse_state *state)
{
#define SKIP_WS do { t = get_token(ts); } while (t.type == TOK_WS)

    char cmd[64];
    token t;
    SKIP_WS;
    if (t.type != '#') {
        die("Unexpected missing hash in processing directive");
    }

    // at this point ts->line should reliably point to our directive line(s)
    const char *remaining = ts->line + ts->ind;
    remaining += strspn(remaining, " \t");

    const int len = strspn(remaining, LOWER);
    snprintf(cmd, sizeof(cmd), "%.*s", len, remaining);
    remaining += len;
    remaining += strspn(remaining, " \t");

    if (strcmp(cmd, "ifdef") == 0) {
        add_if_frame(state);
        handle_ifdef(remaining, state, 0);
    } else if (strcmp(cmd, "ifndef") == 0) {
        add_if_frame(state);
        handle_ifdef(remaining, state, 1);
    } else if (strcmp(cmd, "endif") == 0) {
        handle_endif(state);
        if (output_active) {
            fprintf(state->out, "# %d \"%s\"\n", current_lineno + 1, current_file);
        }
    } else if (strcmp(cmd, "include") == 0) {
        if (output_active) {
            handle_include(remaining, state);
        }
    } else if (strcmp(cmd, "define") == 0) {
        if (output_active) {
            handle_define(remaining, state);
        }
    } else if (strcmp(cmd, "undef") == 0) {
        if (output_active) {
            handle_undef(remaining, state);
        }
    } else if (strcmp(cmd, "if") == 0) {
        add_if_frame(state);
        handle_if(remaining, state);
    } else if (strcmp(cmd, "elif") == 0) {
        handle_if(remaining, state);
    } else if (strcmp(cmd, "else") == 0) {
        handle_else(state);
    } else if (strcmp(cmd, "line") == 0) {
        handle_line_directive(remaining, state);
    } else if (strcmp(cmd, "pragma") == 0) {
        handle_pragma(remaining, state);
    } else if (strcmp(cmd, "warning") == 0) {
        if (output_active) {
            fprintf(stderr, "#warning: %s\n", remaining);
        }
    } else if (strcmp(cmd, "error") == 0) {
        if (output_active) {
            die("#error: %s", remaining);
        }
    } else if (output_active) {
        fprintf(stderr, "Warning: unrecognized directive %s\n", cmd);
    }

    skip_line(ts); // pull next line and update line_is_directive flag
}

static void process_tokens(token_state *ts, parse_state *state)
{
    ignore_list ignored = {0};

    while (!TOKEN_STATE_DIRECTIVE(ts)) {
        // un-ignore all ignored defines once we're no longer reading the macro expansion result
        if (!TOKEN_STATE_READING_IGNORED(ts) && ignored.count) {
            clear_ignore_list(&ignored);
        }

        const token t = get_token(ts);
        if (t.type == EOF) {
            // end of stream
            return;
        }

        if (ts->line_is_directive && !strchr(t.tok, '\n')) {
            // we crossed into a directive, unget the token and stop processing
            push_back_token_data(ts, t.tok);
            return;
        }

        def *d = defines_get(state->defs, t.tok);
        if (d) {
            str_t result = {0};
            if (handle_macro(d, state->defs, ts, &result)) {
                push_back_token_data(ts, result.s);
                add_to_ignore_list(&ignored, d);
            } else {
                fprintf(state->out, "%s", result.s ? result.s : "");
            }
            free_str(&result);
        } else {
            fprintf(state->out, "%s", t.tok);
        }
    }

    if (ignored.count) {
        clear_ignore_list(&ignored);
    }
}
