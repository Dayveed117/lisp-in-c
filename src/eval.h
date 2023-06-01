#ifndef eval_h
#define evan_h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

// Error handling pre-processor function
#define LASSERT(args, cond, err)                                               \
    if (cond)                                                                  \
    {                                                                          \
        lval_del(args);                                                        \
        return lval_err(err);                                                  \
    }

// lval [type] field values
enum
{
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
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
lval *lval_qexpr(void);

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

/* Remove element I from V
Shift remaining list towards the removed element's position */
lval *lval_pop(lval *v, int i);

/* Push X to the front of V
Shifts entire list forward one position */
lval *lval_push(lval *v, lval* x);

/* Apply LVAL_POP on V in index I
Delete the remaining of V */
lval *lval_take(lval *v, int i);

/* Append every element from Y to X */
lval *lval_join(lval *x, lval *y);

/* Append value Y to the front of X */
lval *lval_cons(lval *x, lval *y);

/* Return the first element of V
Delete the remaining of V */
lval *builtin_head(lval *v);

/* Return the last element of V
Delete the remaining of V */
lval *builtin_tail(lval *v);

/* Append value X to V */
lval *builtin_cons(lval *v);

/* Return the number of elements in V
V has to be a Q-Expression */
lval *builtin_len(lval *v);

/* Return all of V except for its final element
V has to be a Q-Expression */
lval *builtin_init(lval *v);

/* Return all of V as a Q-Expression */
lval *builtin_list(lval *v);

/* Join all of V's Q-Expressions */
lval *builtin_join(lval *v);

/* Evaluate all of V */
lval *builtin_eval(lval *v);

/* Evaluate V using symbol OP*/
lval *builtin_op(lval *v, char *op);

/* Apply builtin function or operator FUN to V */
lval *builtin(lval *v, char *fun);

lval *lval_eval_sexpr(lval *v);
lval *lval_eval(lval *v);

#endif
