#ifndef eval_h
#define evan_h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

// lval [type] field values
enum
{
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
};

typedef struct lval
{
    int type;
    long num;
    // Error and Symbol types have some string data
    char *err;
    char *sym;
    // Count keeps track of the number of lists within cell
    int count;
    struct lval **cell;
} lval;

// Construct pointers to new Number, Error, Symbol and Sexpr lvals
lval *lval_num(long num);
lval *lval_err(char *message);
lval *lval_sym(char *symbol);
lval *lval_sexpr(void);

// Deconstruct lval pointers
void lval_del(lval *v);

// Parsing lval
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);

// Print lval
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);

// Evaluating lval
lval *lval_eval_sexpr(lval *v);
lval *lval_eval(lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *builtin_op(lval *v, char *op);

#endif
