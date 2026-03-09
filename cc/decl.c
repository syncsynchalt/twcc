#include "decl.h"
#include "die.h"

#include <assert.h>
#include <strings.h>

static token_type *resolve_params(const ast_node *list);

void add_decl(struct decl **list, int *list_len, const ast_node *node)
{
    struct decl new_decl;
    bzero(&new_decl, sizeof(new_decl));

    if (node->type != NODE_DECL) {
        die("Unexpected node type: %s", print_ast(node));
    }

    int is_extern = 0;
    const ast_node *t = node->left;
    while (t->tok_type == TOK_KW_EXTERN || t->tok_type == TOK_KW_STATIC) {
        is_extern = t->tok_type == TOK_KW_EXTERN;
        t = t->left;
    }
    new_decl.type = t->tok_type;

    assert(node->right->type == NODE_LIST);
    int i;
    ast_node *item;
    for (i = 0; i < node->right->list_len; i++) {
        item = node->right->list[i];
        assert(item->tok_type == '=');
        item = item->left;
        if (item->tok_type == '(') {
            new_decl.is_func = 1;
            new_decl.name = strdup(item->left->s);
            new_decl.params = resolve_params(item->right);
            new_decl.params_len = item->right->list_len;
        } else {
            new_decl.is_func = 0;
            new_decl.name = strdup(item->s);
            new_decl.params = NULL;
            new_decl.params_len = 0;
        }
        new_decl.is_extern = is_extern;

        if (*list_len % 5 == 0) {
            *list = realloc(*list, sizeof(struct decl) * (*list_len + 5));
        }
        memcpy(&(*list)[*list_len], &new_decl, sizeof(struct decl));
        *list_len += 1;
    }
}

struct decl *get_decl(struct decl *list, int list_len, const char *name)
{
    int i;
    for (i = 0; i < list_len; i++) {
        if (strcmp(list[i].name, name) == 0) {
            return &list[i];
        }
    }
    return NULL;
}

static token_type *resolve_params(const ast_node *list)
{
    token_type *ret = calloc(1, sizeof(token_type) * 20);
    int reti = 0;

    int i;
    for (i = 0; i < list->list_len; i++) {
        const ast_node *item = list->list[i];
        while (item->tok_type == TOK_KW_CONST) {
            item = item->right;
        }
        ret[reti++] = item->tok_type;
    }
    return ret;
}
