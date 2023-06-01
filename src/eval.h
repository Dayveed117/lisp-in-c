#ifndef eval_h
#define evan_h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

// TODO : Modify LASSERTS in function definitions
// Error handling pre-processor function
#define LASSERT(args, cond, fmt, ...)                                          \
    if (cond)                                                                  \
    {                                                                          \
        lval *err = lval_err(fmt, ##__VA_ARGS__);                              \
        lval_del(args);                                                        \
        return err;                                                            \
    }

// lval [type] field values
enum
{
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR,
};

typedef struct lval lval;
typedef struct lenv lenv;
typedef lval *(*lbuiltin)(lenv *, lval *);

struct lval
{
    int type;
    long num;
    // Error and Symbol types have some string data
    char *err;
    char *sym;
    // Pointer to a function in the context
    lbuiltin fun;
    // Count keeps track of the number of lists within cell
    int count;
    struct lval **cell;
};

struct lenv
{
    // Double list of matching lengths
    int count;
    // Stores keys or ids
    char **syms;
    // Stores values
    lval **vals;
};

lenv *lenv_new(void);
void lenv_del(lenv *e);

/* Attempt to find value of V inside environment E
Return value if found, return error otherwise */
lval *lenv_get(lenv *e, lval *v);

/* Associate symbol K with value from V
if K already exists, update with V */
int lenv_put(lenv *e, lval *k, lval *v);

void lenv_add_builtin(lenv *e, char *id, lbuiltin func);

void lenv_add_builtins(lenv *e);

/* --------------------------------------- */
/* ---------- LVAL DECLARATIONS ---------- */
/* --------------------------------------- */

// Construct pointers to new Number, Error, Symbol and Sexpr lvals
lval *lval_num(long num);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *symbol);
lval *lval_fun(lbuiltin func);
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
char* ltype_name(int t);

/* Remove element I from V
Shift remaining list towards the removed element's position */
lval *lval_pop(lval *v, int i);

/* Push X to the front of V
Shifts entire list forward one position */
lval *lval_push(lval *v, lval *x);

/* Return deep copy of V*/
lval *lval_copy(lval *v);

/* Apply LVAL_POP on V in index I
Delete the remaining of V */
lval *lval_take(lval *v, int i);

/* Append every element from Y to X */
lval *lval_join(lval *x, lval *y);

/* Append value Y to the front of X */
lval *lval_cons(lval *x, lval *y);

/* --------------------------------------------------------- */
/* ---------- List Manipulation Builtin Functions ---------- */
/* --------------------------------------------------------- */

/* Return the first element of V
Delete the remaining of V */
lval *builtin_head(lenv *e, lval *v);

/* Return the last element of V
Delete the remaining of V */
lval *builtin_tail(lenv *e, lval *v);

/* Append value X to V */
lval *builtin_cons(lenv *e, lval *v);

/* Return the number of elements in V
V has to be a Q-Expression */
lval *builtin_len(lenv *e, lval *v);

/* Return all of V except for its final element
V has to be a Q-Expression */
lval *builtin_init(lenv *e, lval *v);

/* Return all of V as a Q-Expression */
lval *builtin_list(lenv *e, lval *v);

/* Join all of V's Q-Expressions */
lval *builtin_join(lenv *e, lval *v);

/* Evaluate all of V */
lval *builtin_eval(lenv *e, lval *v);

/* Define symbol V in environment E */
lval *builtin_def(lenv *e, lval *v);

/* -------------------------------------------------- */
/* ---------- Arithmetic Builtin Functions ---------- */
/* -------------------------------------------------- */

lval *builtin_op(lenv *e, lval *v, char *op);
lval *builtin_add(lenv *e, lval *v);
lval *builtin_sub(lenv *e, lval *v);
lval *builtin_mul(lenv *e, lval *v);
lval *builtin_div(lenv *e, lval *v);
lval *builtin_mod(lenv *e, lval *v);
lval *builtin_pow(lenv *e, lval *v);
lval *builtin_min(lenv *e, lval *v);
lval *builtin_max(lenv *e, lval *v);

lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);

#endif
