#include "asm.h"
#include "data_section.h"
#include "decl.h"
#include "die.h"
#include "regs.h"
#include "typing.h"
#include <string.h>
#include <ctype.h>

static void print_globals(const ast_node *ast);
static void handle_function(const ast_node *type, const char *name, const ast_node *statements);
static void handle_call(const ast_node *call);
static reg_t resolve_statement(regs_in_use_t in_use, const ast_node *s);
static void print_text_section();

FILE *asmout;
int flt_constant_ctr;
char **strings = NULL;
int strings_len = 0;
char **externs = NULL;
int externs_len = 0;
struct decl *decl_list = NULL;
int decl_list_len = 0;

static int add_string(const char *s)
{
    if (strings_len % 5 == 0) {
        strings = realloc(strings, sizeof(*strings) * (strings_len + 5));
        strings[strings_len] = strdup(s);
    }
    return strings_len++;
}

static void load_str_reg(const int reg, const int str_index, const char *hint)
{
    int i;
    fprintf(asmout, "    adrp x%d, _%d.str@PAGE ; \"", reg, str_index);
    for (i = 0; i < 10; i++) {
        fprintf(asmout, "%c", isprint(hint[i]) ? hint[i] : '.');
    }
    if (i == 10) {
        fprintf(asmout, "...");
    }
    fprintf(asmout, "\"\n");
    fprintf(asmout, "    add  x%d, x%d, _%d.str@PAGEOFF\n", reg, reg, str_index);
}

void read_ast(const ast_node *ast, FILE *out)
{
    asmout = out;

    if (ast->type != NODE_TRANSLATION_UNIT) {
        die("Not a translation unit");
    }

    print_globals(ast);
    fprintf(asmout, "\n");

    int i;
    for (i = 0; i < ast->list_len; i++) {
        const ast_node *item = ast->list[i];
        if (item->type == NODE_DECL) {
            add_decl(&decl_list, &decl_list_len, item);
        }
        else if (item->type == NODE_FUNCTION) {
            const ast_node *ret = item->left;
            const char *nam = item->right->left->s;
            const ast_node *statements = item->list[0];
            handle_function(ret, nam, statements);
        }
    }

    print_data_section(asmout);
    print_text_section();
}

static void print_globals(const ast_node *ast)
{
    int printed_global = 0;

    int i;
    for (i = 0; ast->list && ast->list[i]; i++) {
        const ast_node *item = ast->list[i];
        if (item->type == NODE_FUNCTION) {
            if (!printed_global) {
                fprintf(asmout, ".global ");
                printed_global = 1;
            } else {
                fprintf(asmout, ", ");
            }

            const ast_node *nam = item->right;
            fprintf(asmout, "_%s", nam->left->s);
        }
        if (printed_global) {
            fprintf(asmout, "\n");
        }
    }
}

static reg_t convert_to_float(regs_in_use_t in_use, reg_t ir)
{
    if (!ir.ireg) {
        // no conversion needed
        return ir;
    }
    reg_t fr = alloc_flt_register(in_use);
    fprintf(asmout, "    scvtf d%d, x%d // convert signed int to float\n", fr.freg, ir.ireg);
    return fr;
}

static reg_t convert_to_sint(regs_in_use_t in_use, reg_t fr)
{
    if (!fr.freg) {
        // no conversion needed
        return fr;
    }
    reg_t ir = alloc_int_register(in_use);
    fprintf(asmout, "    fcvtzs x%d, d%d // convert float to signed int\n", ir.ireg, fr.freg);
    return ir;
}

static void handle_return(const ast_node *type_info, const ast_node *statement, const int call_exit)
{
    regs_in_use_t in_use = {0};
    type_t t = read_type(type_info);

    if (t == TYPE_VOID) {
        if (statement->left) {
            die("Return value in void function");
        }
        fprintf(asmout, "    %s\n\n", call_exit ? "bl _exit" : "ret");
    } else {
        if (!statement->left) {
            die("No return value in non-void function");
        }
        reg_t reg = resolve_statement(in_use, statement->left);
        if (t == TYPE_INT) {
            reg = convert_to_sint(in_use, reg);
            fprintf(asmout, "    mov x0, x%d\n", reg.ireg);
        } else if (t == TYPE_FLT) {
            reg = convert_to_float(in_use, reg);
            fprintf(asmout, "    fmov d0, d%d\n", reg.freg);
        }
        fprintf(asmout, "    %s\n\n", call_exit ? "bl _exit" : "ret");
    }
}

static void handle_function(const ast_node *type, const char *name, const ast_node *statements)
{
    fprintf(asmout, "_%s:\n", name);
    int i;
    for (i = 0; statements->list[i]; i++) {
        const ast_node *s = statements->list[i];
        if (s->tok_type == TOK_KW_RETURN) {
            handle_return(type, s, strcmp(name, "main") == 0);
        } else if (s->tok_type == '(') {
            // todo - capture lval
            handle_call(s);
        } else {
            die("xxx not handled: %s", print_ast(s));
        }
    }
}

static void handle_call(const ast_node *call)
{
    const char *name = call->left->s;
    const struct decl *func_ref = get_decl(decl_list, decl_list_len, name);
    if (!func_ref) {
        die("Function %s not found: %s", name, print_ast(call));
    }

    const ast_node *params = call->right;
    if (params->list_len != func_ref->params_len) {
        die("Mismatched params length %d (expected %d): %s",
            params->list_len, func_ref->params_len, print_ast(call));
    }
    if (params->list_len > 8) {
        die("Too many registers needed, not handled. %s", print_ast(call));
    }

    int i, str_index;
    for (i = 0; params->list[i]; i++) {
        if (params->list[i]->type == NODE_STR) {
            str_index = add_string(params->list[i]->s);
            load_str_reg(i, str_index, params->list[i]->s);
        } else {
            die("Not handled type %d. %s", params->list[i]->type, print_ast(call));
        }
    }

    fprintf(asmout, "    bl _%s\n", name);
}

static reg_t resolve_statement(regs_in_use_t in_use, const ast_node *s)
{
    reg_t reg1, reg2, reg3, reg4;

    switch ((int)s->tok_type) {
        case TOK_NUM:
            if (s->type == NODE_INT) {
                if (s->ival <= 0xffff) {
                    reg1 = alloc_int_register(in_use);
                    fprintf(asmout, "    mov x%d, #0x%lx\n", reg1.ireg, s->ival);
                    return reg1;
                } else {
                    die("xxx int size not yet handled");
                }
            } else if (s->type == NODE_FLT) {
                reg1 = alloc_flt_register(in_use);
                reg2 = alloc_int_register(in_use);
                add_to_data_section("flt_const_%d: .double %lf", ++flt_constant_ctr, s->fval);
                fprintf(asmout, "    adrp x%d, flt_const_%d@PAGE // load the address for constant %f\n",
                    reg2.ireg, flt_constant_ctr, s->fval);
                fprintf(asmout, "    ldr d%d, [x%d, flt_const_%d@PAGEOFF] // copy the constant into register\n",
                    reg1.freg, reg2.ireg, flt_constant_ctr);
                return reg1;
            }
            die("xxx %s:%d", __FILE__, __LINE__);
        case '+':
            reg1 = resolve_statement(in_use, s->left);
            mark_register(&in_use, reg1);
            reg2 = resolve_statement(in_use, s->right);
            if (reg1.freg || reg2.freg) {
                // add floats, promote if necessary
                // todo test me
                reg3 = convert_to_float(in_use, reg1);
                mark_register(&in_use, reg3);
                reg4 = convert_to_float(in_use, reg2);
                fprintf(asmout, "    fadd d%d, d%d, d%d\n", reg3.freg, reg3.freg, reg4.freg);
                return reg3;
            } else {
                // add two ints
                fprintf(asmout, "    add x%d, x%d, x%d\n", reg1.ireg, reg1.ireg, reg2.ireg);
                return reg1;
            }
        default:
            die("xxx");
    }
}

static void print_text_section()
{
    fprintf(asmout, "\n\n.text\n");
    int i, j;
    size_t len;
    for (i = 0; i < strings_len; i++) {
        fprintf(asmout, "_%d.str: .asciz \"", i);
        len = strlen(strings[i]);
        for (j = 0; j < len; j++) {
            if (isprint(strings[i][j])) {
                fprintf(asmout, "%c", strings[i][j]);
            } else if (strings[i][j] == '\n') {
                fprintf(asmout, "\\n");
            } else {
                fprintf(asmout, "\\%03o", (unsigned char)strings[i][j]);
            }
        }
        fprintf(asmout, "\"\n");
    }
}
