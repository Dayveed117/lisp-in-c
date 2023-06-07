#ifndef eval_h
#define evan_h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

// Main error handling pre-processor expansion
#define LASSERT(args, cond, fmt, ...)                                          \
    if (!(cond))                                                               \
    {                                                                          \
        lval *err = lval_err(fmt, ##__VA_ARGS__);                              \
        lval_del(args);                                                        \
        return err;                                                            \
    }

// Expansion for assuring correct number of arguments in function
#define LASSERT_NUMARGS(func, args, num)                                       \
    LASSERT(args, args->count == num,                                          \
            "Function %s non compatible arity -- Got %d, Expected %d", func,   \
            args->count, num);

// Expansion for assuring correct argument types in function
#define LASSERT_TYPE(func, args, index, expect)                                \
    LASSERT(args, args->cell[index]->type == expect,                           \
            "Function %s passed incorrect type for argument %d -- Got %s, "    \
            "Expected %s",                                                     \
            func, index + 1, ltype_name(args->cell[index]->type),              \
            ltype_name(expect));

// Expansion for assuring operating on non-empty list
#define LASSERT_NON_EMPTY(func, args, index)                                   \
    LASSERT(args, args->cell[index]->count != 0,                               \
            "Function %s expects non-empty list for argument %d", func,        \
            index);

// lval [type] field valuesk
enum
{
    LVAL_NUM,
    LVAL_BOOL,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR,
};

// TODO : Change ordering and equality checks with bool, if applicable
// TODO : Represent it in the language
typedef enum { false, true } bool;

typedef struct lval lval;
typedef struct lenv lenv;
typedef lval *(*lbuiltin)(lenv *, lval *);

struct lval
{
    int type;
    bool bool;
    long num;
    char *err;
    char *sym;

    /* Pointer to a function in the context
    If NULL, it is user-defined function, builtin otherwise */
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

    // Number of lists within cell field
    int count;
    // List of pointers to other lval pointers
    struct lval **cell;
};

struct lenv
{
    /* Pointer to parent environment
    If NULL, then the current environment is the global one */
    lenv *parent;
    // Double list of matching lengths
    int count;
    // Stores keys
    char **syms;
    // Stores values
    lval **vals;
};

/* --------------------------------------- */
/* ---------- LENV DECLARATIONS ---------- */
/* --------------------------------------- */

/* Construct a new environment */
lenv *lenv_new(void);

/* Free an environment */
void lenv_del(lenv *e);

/* Attempt to find value of V inside environment E
Return value if found, return error otherwise */
lval *lenv_get(lenv *e, lval *v);

/* Reverse look up lenv_get */
lval *lenv_get_key(lenv *e, lval *k);

/* Return deep copy of environment E */
lenv *lenv_copy(lenv *e);

/* Print all registered symbols */
void lenv_print_definitions(lenv *e);

/* Associate symbol K with value from V in environment E
If K already exists, update with V */
int lenv_put(lenv *e, lval *k, lval *v);

// Call LENV_PUT on global environment E
int lenv_def(lenv *e, lval *k, lval *v);

/* Bind key ID to function FUNC */
void lenv_add_builtin(lenv *e, char *id, lbuiltin func);

/* Group adding builtin functions */
void lenv_add_builtins(lenv *e);

/* --------------------------------------- */
/* ---------- LVAL DECLARATIONS ---------- */
/* --------------------------------------- */

// Construct pointers to types of lval
lval *lval_empty(void);
lval *lval_num(long num);
lval *lval_bool(bool bool);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *symbol);
lval *lval_fun(lbuiltin func);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_sexpr(void);
lval *lval_qexpr(void);

// Deconstruct lval pointers
void lval_del(lval *v);

// Parsing lval
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);

// Print lval
void lval_expr_print(lenv *e, lval *v, char open, char close);
void lval_print_func(lenv *e, lval *v);
void lval_print(lenv *e, lval *v);
void lval_println(lenv *e, lval *v);
char *ltype_name(int t);

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

/* Substitute ARGS for PARAMS in environment E
Evaluation may be done partially */
lval *lval_call(lenv *e, lval *params, lval *args);

/* Equality check between entirety of X and Y */
bool lval_eq(lval *x, lval *y);

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

/* Local Assignment of V in E */
lval *builtin_put(lenv *e, lval *v);

/* Master Assignment */
lval *builtin_var(lenv *e, lval *v, char *func);

/* User-defined functions */
lval *builtin_lambda(lenv *e, lval *v);

/* Better way to define user functions */
lval *builtin_fun(lenv *e, lval *v);

/* ---------------------------------------------------------- */
/* ---------- Arithmetic & Logic Builtin Functions ---------- */
/* ---------------------------------------------------------- */

lval *builtin_op(lenv *e, lval *v, char *op);
lval *builtin_add(lenv *e, lval *v);
lval *builtin_sub(lenv *e, lval *v);
lval *builtin_mul(lenv *e, lval *v);
lval *builtin_div(lenv *e, lval *v);
lval *builtin_mod(lenv *e, lval *v);
lval *builtin_pow(lenv *e, lval *v);
lval *builtin_min(lenv *e, lval *v);
lval *builtin_max(lenv *e, lval *v);

lval *builtin_ord(lenv *e, lval *v, char *op);
lval *builtin_gt(lenv *e, lval *v);
lval *builtin_lt(lenv *e, lval *v);
lval *builtin_ge(lenv *e, lval *v);
lval *builtin_le(lenv *e, lval *v);
lval *builtin_cmp(lenv *e, lval *v, char *op);
lval *builtin_eq(lenv *e, lval *v);
lval *builtin_neq(lenv *e, lval *v);

lval *builtin_true(lenv *e, lval *v);
lval *builtin_false(lenv *e, lval *v);

lval *builtin_if(lenv *e, lval *v);
lval *builtin_and(lenv *e, lval *v);
lval *builtin_or(lenv *e, lval *v);
lval *builtin_not(lenv *e, lval *v);

lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);

#endif
