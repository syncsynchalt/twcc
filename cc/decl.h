#ifndef INC_DECL_H
#define INC_DECL_H

#include "ast.h"

struct decl {
    char *name;
    int is_func;
    int is_extern;
    token_type type;
    token_type *params;
    int params_len;
};

extern void add_decl(struct decl **list, int *list_len, const ast_node *node);
extern struct decl *get_decl(struct decl *list, int list_len, const char *name);

#endif // INC_DECL_H
