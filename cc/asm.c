#include "asm.h"
#include "typing.h"
#include "regs.h"
#include "data_section.h"
#include "die.h"

static void print_globals(const ast_node *ast);
static void handle_function(const ast_node *type, const ast_node *name, const ast_node *statements);
static reg_t resolve_statement(regs_in_use_t in_use, const ast_node *s);

FILE *asmout;
int flt_constant_ctr;

void read_ast(const ast_node *ast, FILE *out)
{
    asmout = out;

    if (ast->type != NODE_TRANSLATION_UNIT) {
        die("Not a translation unit");
    }

    print_globals(ast);
    fprintf(asmout, "\n");

    int i;
    for (i = 0; ast->list && ast->list[i]; i++) {
        const ast_node *item = ast->list[i];
        if (item->type == NODE_DECL) {
            // todo - implement
        }
        else if (item->type == NODE_FUNCTION) {
            const ast_node *ret = item->left;
            const ast_node *nam = item->right;
            const ast_node *statements = item->list[0];
            handle_function(ret, nam, statements);
        }
    }

    print_data_section(asmout);
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

static void handle_return(const ast_node *type_info, const ast_node *statement)
{
    regs_in_use_t in_use = {0};
    type_t t = read_type(type_info);

    if (t == TYPE_VOID) {
        if (statement->left) {
            die("Return value in void function");
        }
        fprintf(asmout, "    ret\n\n");
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
        fprintf(asmout, "    ret\n\n");
    }
}

static void handle_function(const ast_node *type, const ast_node *name, const ast_node *statements)
{
    fprintf(asmout, "_%s:\n", name->left->s);
    int i;
    for (i = 0; statements->list[i]; i++) {
        const ast_node *s = statements->list[i];
        if (s->tok_type == TOK_KW_RETURN) {
            handle_return(type, s);
        } else {
            die("xxx not handled");
        }
    }
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
