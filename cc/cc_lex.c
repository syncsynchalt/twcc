#include "cc_lex.h"
#include <string.h>
#include <errno.h>

#include "common.h"
#include "die.h"
#include "parse.h"
#include "hashmap.h"

ast_node *cc_parse_result;

static char *lex_line = NULL;
static size_t lex_line_size;
static size_t lex_line_index;
static size_t lex_line_end;

static char token_buf[TOKEN_BUF_SZ];

static hashmap *type_names;

static int cc_getch(void *unused);
static int cc_ungetch(int c, void *unused);
static int read_next_line(void);

int yylex(void)
{
    if (!type_names) {
        type_names = hashmap_init(16);
    }

    if (lex_line_index >= lex_line_end) {
        if (read_next_line() < 0) {
            yylval = NULL;
            return -1;
        }
    }

    token t;
    do {
        t = read_token(cc_getch, cc_ungetch, NULL, token_buf);
    } while (t.type == TOK_WS);

    if (t.type == TOK_EOF) {
        yylval = NULL;
        return TOK_EOF;
    }
    if (t.type == TOK_ID && hashmap_get(type_names, t.tok)) {
        t.type = TOK_TYPE_NAME;
    }

    yylval = make_ast_node(t, NULL, NULL);
    return t.type;
}

void yyerror(const char *s)
{
    fprintf(stderr, "%s\n", s);
    int i;
    fprintf(stderr, "line: %s", lex_line);
    if (!strchr(lex_line, '\n')) {
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "     ");
    for (i = 0; i < lex_line_index; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
    exit(1);
}

static char filename_tmp[256];

static int read_next_line(void)
{
    for (;;) {
        lex_line_end = getline(&lex_line, &lex_line_size, cc_input);
        if (lex_line_end == EOF) {
            if (feof(cc_input)) {
                return -1;
            }
            die("Error reading input file: %s", strerror(errno));
        }
        lex_line_index = 0;
        if (lex_line[0] == '#') {
            const char *p = lex_line;
            p += strspn(p, "# ");
            cc_lineno = (int)strtol(p, NULL, 10);
            p += strspn(p, NUMERIC " ");;
            decode_str(p, filename_tmp, sizeof(filename_tmp));
            cc_filename = filename_tmp;
        } else {
            break;
        }
    }
    return 0;
}

static int cc_getch(void *unused)
{
    if (lex_line_index >= lex_line_end) {
        if (read_next_line() < 0) {
            return EOF;
        }
    }
    return lex_line[lex_line_index++];
}

// ReSharper disable once CppParameterMayBeConst
static int cc_ungetch(int c, void *unused)
{
    if (!lex_line_index) {
        die("Tried to ungetc past start of line");
    }
    if (lex_line[--lex_line_index] != c) {
        die("Tried to unget character %c (%02x) when last character was %c (%02x)",
            c, c, lex_line[lex_line_index], lex_line[lex_line_index]);
    }
    return 0;
}

void check_for_typedef(const ast_node *node)
{
    // left contains TYPEDEF, right is a list with names on left
    if (contains_token(node->left, TOK_KW_TYPEDEF)) {
        if (!node->right || !node->right->list_len) {
            die("Unexpected lack of list on right of typedef decl");
        }
        size_t i;
        for (i = 0; i < node->right->list_len; i++) {
            char *n = node->right->list[i]->left->s;
            hashmap_add(type_names, n, n);
        }
    }
}
