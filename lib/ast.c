#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "str.h"
#include "die.h"

ast_node *make_ast_node(const token t, ast_node *left, ast_node *right)
{
    char buf[256];

    // todo - track so we can free
    ast_node *node = calloc(1, sizeof *node);
    node->left = left;
    node->right = right;

    node->tok_type = t.type;
    if (IS_KEYWORD(t.type) || t.type == TOK_ID || t.type == TOK_TYPE_NAME) {
        node->s = strdup(t.tok);
        node->type = NODE_OTHER;
    } else switch ((int)t.type) {
        case TOK_NUM:
            snprintf(buf, sizeof(buf), "%s", t.tok);
            // todo - handle type suffixes
            if (strchr(t.tok, '.') || strchr(t.tok, 'e') || strchr(t.tok, 'E')) {
                node->type = NODE_FLT;
                node->fval = strtod(buf, NULL);
            } else {
                node->type = NODE_INT;
                node->ival = strtol(t.tok, NULL, 0);
            }
            break;
        case TOK_STR:
            node->s = malloc(strlen(t.tok) + 1);
            decode_str(t.tok, node->s, strlen(t.tok) + 1);
            node->type = NODE_STR;
            break;
        case TOK_CHA:
            node->cval = t.tok[1] == '\\' ? t.tok[2] : t.tok[1];
            node->type = NODE_CHA;
            break;
        default:
            node->type = NODE_OTHER;
            break;
    }
    return node;
}

void add_to_ast_list(ast_node *node, ast_node *list_member)
{
    if (node->list_len % 5 == 0) {
        node->list = realloc(node->list, (node->list_len + 6) * sizeof(ast_node *));
    }
    node->list[node->list_len++] = list_member;
    node->list[node->list_len] = NULL;
}

void free_ast_node(ast_node *node)
{
    free(node->s);
    free(node);
}

int contains_token(const ast_node *node, token_type t)
{
    if (!node) {
        return 0;
    }
    return (node->left && contains_token(node->left, t)) ||
        (node->right && contains_token(node->right, t)) ||
        node->tok_type == t;
}

#if 0
std::string print_ast_node(const ast_node *node)
{
    std::string result;

    if (node->list_len) {
        result += "[";
        for (auto i = 0; i < node->list_len; i++) {
            result += print_ast_node(node->list[i]);
        }
        result += "]";
    }
    return result;
}

std::string print_ast_inner(const ast_node *node, int indent)
{
    auto result = std::string();

    if (node->type == NODE_INT) {
        return std::to_string(node->ival);
    } else if (node->type == NODE_FLT) {
        return std::to_string(node->fval);
    } else if (node->type == NODE_CHA) {
        if (!isprint(node->cval)) {
            return "'?'"s;
        }
        if (node->cval == '"' || node->cval == '\'') {
            return "'\\"s + (char)node->cval + "'";
        }
        return "'"s + (char)node->cval + "'";
    } else if (node->type == NODE_STR) {
        const std::stringstream ss(node->s);
        return ss.str();
    }

    result += "(";
    if (node->left) {
        result += print_ast(node->left);
    }

    result += print_ast_node_type(node);
    result += print_ast_node(node);

    if (node->right) {
        result += print_ast(node->right);
    }

    result += ")";
    return result;
}
#endif

static void add_indent(const int indent, str_t *out)
{
    int i;
    for (i = 0; i < indent; i++) {
        add_to_str(out, "  ");
    }
}

static char *get_ast_node_type(const ast_node *node)
{
    switch (node->type) {
        case NODE_LIST: return "(nt:list)";
        case NODE_SUBSCRIPT: return "(nt:subscript)";
        case NODE_CAST: return "(nt:cast)";
        case NODE_ID_LIST: return "(nt:id_list)";
        case NODE_INIT_LIST: return "(nt:init_list)";
        case NODE_DECL: return "(nt:decl)";
        case NODE_STRUCT_MEMBERS: return "(nt:struct_members)";
        case NODE_TRANSLATION_UNIT: return "(nt:translation_unit)";
        case NODE_BITFIELD: return "(nt:bitfield)";
        case NODE_PARAM_LIST: return "(nt:param_list)";
        case NODE_FUNCTION: return "(nt:function)";
        case NODE_ABSTRACT: return "(nt:abstract)";
        case NODE_ABSTRACT_TYPE: return "(nt:abstract_type)";
        case NODE_OTHER:
        case NODE_INT:
        case NODE_FLT:
        case NODE_STR:
        case NODE_CHA:
            return "";
        default:
            break;
    }
    die("Unknown node type %d", node->type);
}

static char *get_ast_token_name(const ast_node *node)
{
    switch (node->tok_type) {
        case TOK_WS: return " ";
        case TOK_KW_AUTO: return "auto";
        case TOK_KW_BREAK: return "break";
        case TOK_KW_CASE: return "case";
        case TOK_KW_CHAR: return "char";
        case TOK_KW_CONST: return "const";
        case TOK_KW_CONTINUE: return "continue";
        case TOK_KW_DEFAULT: return "default";
        case TOK_KW_DO: return "do";
        case TOK_KW_DOUBLE: return "double";
        case TOK_KW_ELSE: return "else";
        case TOK_KW_ENUM: return "enum";
        case TOK_KW_EXTERN: return "extern";
        case TOK_KW_FLOAT: return "float";
        case TOK_KW_FOR: return "for";
        case TOK_KW_GOTO: return "goto";
        case TOK_KW_IF: return "if";
        case TOK_KW_INT: return "int";
        case TOK_KW_LONG: return "long";
        case TOK_KW_REGISTER: return "register";
        case TOK_KW_RETURN: return "return";
        case TOK_KW_SHORT: return "short";
        case TOK_KW_SIGNED: return "signed";
        case TOK_KW_SIZEOF: return "sizeof";
        case TOK_KW_STATIC: return "static";
        case TOK_KW_STRUCT: return "struct";
        case TOK_KW_SWITCH: return "switch";
        case TOK_KW_TYPEDEF: return "typedef";
        case TOK_KW_UNION: return "union";
        case TOK_KW_UNSIGNED: return "unsigned";
        case TOK_KW_VOID: return "void";
        case TOK_KW_VOLATILE: return "volatile";
        case TOK_KW_WHILE: return "while";
        case TOK_ELLIPSIS: return "...";
        case TOK_PTR_OP: return "->";
        case TOK_LEFT_OP: return "<<";
        case TOK_RIGHT_OP: return ">>";
        case TOK_AND_OP: return "&&";
        case TOK_OR_OP: return "||";
        case TOK_EQ_OP: return "==";
        case TOK_NE_OP: return "!=";
        case TOK_LE_OP: return "<=";
        case TOK_GE_OP: return ">=";
        case TOK_INC_OP: return "++";
        case TOK_DEC_OP: return "--";
        case TOK_ADD_ASSIGN: return "+=";
        case TOK_SUB_ASSIGN: return "-=";
        case TOK_MUL_ASSIGN: return "*=";
        case TOK_DIV_ASSIGN: return "/=";
        case TOK_LEFT_ASSIGN: return "<<=";
        case TOK_RIGHT_ASSIGN: return ">>=";
        case TOK_MOD_ASSIGN: return "%=";
        case TOK_AND_ASSIGN: return "&=";
        case TOK_OR_ASSIGN: return "|=";
        case TOK_XOR_ASSIGN: return "^=";
        case TOK_PP_COMBINE: return "##";
        case TOK_PP_CONTINUE: return "\\\n";
        case TOK_COMMENT: return "/*comment*/";
        case TOK_LINE_COMMENT: return "//line-comment\n";
        default:
            die("Unexpected token type: %d", node->tok_type);
    }
}

static void add_ast_token(const ast_node *node, str_t *out)
{
    char buf[10];
    if (node->type == NODE_CHA) {
        snprintf(buf, sizeof(buf), "CHA:%c", node->cval);
        add_to_str(out, buf);
    } else if (node->type == NODE_INT) {
        snprintf(buf, sizeof(buf), "INT:%ld", node->ival);
        add_to_str(out, buf);
    } else if (node->type == NODE_FLT) {
        snprintf(buf, sizeof(buf), "FLT:%f", node->fval);
        add_to_str(out, buf);
    } else if (node->type == NODE_STR) {
        snprintf(buf, sizeof(buf), "\"%s\"", node->s);
        add_to_str(out, buf);
    } else if (node->tok_type == TOK_ID) {
        add_to_str(out, "ID:");
        add_to_str(out, node->s);
    } else if (node->tok_type == TOK_TYPE_NAME) {
        add_to_str(out, "TYPENAME:");
        add_to_str(out, node->s);
    } else if (node->tok_type == '(') {
        add_to_str(out, "LPAREN");
    } else if (node->tok_type == ')') {
        add_to_str(out, "RPAREN");
    } else if (node->tok_type < 256) {
        snprintf(buf, 10, "%c", node->tok_type);
        add_to_str(out, buf);
    } else {
        add_to_str(out, get_ast_token_name(node));
    }
}

static void add_ast_node(const ast_node *node, int indent, str_t *out);

static void add_ast_list(const ast_node *node, const int indent, str_t *out)
{
    if (!node->list_len) {
        return;
    }

    int i;
    add_indent(indent, out);
    add_to_str(out, "[\n");
    for (i = 0; i < node->list_len; i++) {
        add_indent(indent + 1, out);
        add_ast_node(node->list[i], indent + 1, out);
        add_to_str(out, "\n");
    }
    add_indent(indent, out);
    add_to_str(out, "]\n");
}

static void add_ast_node(const ast_node *node, const int indent, str_t *out)
{
    if (!node) {
        return;
    }
    if (!node->left && !node->right && !node->list_len) {
        add_to_str(out, get_ast_node_type(node));
        add_ast_token(node, out);
    } else {
        add_to_str(out, "(\n");
        add_indent(indent+1, out);
        add_to_str(out, get_ast_node_type(node));
        add_ast_token(node, out);
        add_to_str(out, "\n");
        if (node->left || node->right) {
            add_indent(indent+1, out);
            node->left ? add_ast_node(node->left, indent + 1, out) : add_to_str(out, "NULL");
            add_to_str(out, "\n");
            add_indent(indent+1, out);
            node->right ? add_ast_node(node->right, indent + 1, out) : add_to_str(out, "NULL");
            add_to_str(out, "\n");
        }
        if (node->list_len) {
            add_ast_list(node, indent + 1, out);
        }
        add_indent(indent, out);
        add_to_str(out, ")");
    }
}

char *print_ast(const ast_node *node)
{
    str_t out = {0};
    add_ast_node(node, 0, &out);
    if (out.s && out.s[0] == '(') {
        add_to_str(&out, "\n");
    }
    return out.s;
}
