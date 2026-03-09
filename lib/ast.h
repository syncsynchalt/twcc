#pragma once
#include "lex.h"

typedef enum {
    NODE_OTHER,
    NODE_INT,
    NODE_FLT,
    NODE_STR,
    NODE_CHA,

    NODE_LIST,
    NODE_SUBSCRIPT,
    NODE_CAST,
    NODE_ID_LIST,
    NODE_INIT_LIST,
    NODE_DECL,
    NODE_STRUCT_MEMBERS,
    NODE_TRANSLATION_UNIT, // root node
    NODE_BITFIELD,
    NODE_PARAM_LIST,
    NODE_FUNCTION,

    NODE_ABSTRACT,
    NODE_ABSTRACT_TYPE,
} node_type;

typedef struct ast_node_ {
    node_type type;
    token_type tok_type;

    char *s;
    unsigned long ival;
    double fval;
    unsigned char cval;

    struct ast_node_ *left;
    struct ast_node_ *right;

    struct ast_node_ **list;
    int list_len;
} ast_node;

extern ast_node *make_ast_node(const token t, ast_node *left, ast_node *right);
extern void add_to_ast_list(ast_node *node, ast_node *list_member);
extern void free_ast_node(ast_node *node);
extern int contains_token(const ast_node *node, token_type t);
extern char *print_ast(const ast_node *node);
