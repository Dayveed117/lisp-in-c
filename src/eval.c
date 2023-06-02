#include "eval.h"

/* ------------------------------------ */
/* ---------- LENV Functions ---------- */
/* ------------------------------------ */

lenv *lenv_new(void)
{
    lenv *e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;

    return e;
}

void lenv_del(lenv *e)
{
    for (size_t i = 0; i < e->count; i++)
    {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval *lenv_get(lenv *e, lval *k)
{
    for (size_t i = 0; i < e->count; i++)
        if (strcmp(e->syms[i], k->sym) == 0)
            return lval_copy(e->vals[i]);

    // Climb parent hierarchy to find symbol
    if (e->parent)
        return lenv_get(e->parent, k);

    return lval_err("Undefined Symbol '%s'", k->sym);
}

lval *lenv_get_key(lenv *e, lval *k)
{
    for (size_t i = 0; i < e->count; i++)
        // ! Very fishy, might require adaption for finding lambdas
        if (e->vals[i]->builtin != NULL && e->vals[i]->builtin == k->builtin)
            return lval_sym(e->syms[i]);

    // Climb parent hierarchy to find symbol
    if (e->parent)
        lenv_get_key(e->parent, k);

    return lval_err("Function not found");
}

lenv *lenv_copy(lenv *e)
{
    lenv *ne = malloc(sizeof(lenv));
    ne->parent = e->parent;
    ne->count = e->count;
    ne->syms = e->syms;
    ne->vals = e->vals;
    for (size_t i = 0; i < ne->count; i++)
    {
        ne->syms[i] = malloc(strlen(ne->syms[i]) + 1);
        strcpy(ne->syms[i], e->syms[i]);
        ne->vals[i] = lval_copy(e->vals[i]);
    }

    return ne;
}

int lenv_put(lenv *e, lval *k, lval *v)
{
    // If K exists in E, update value with new V
    for (size_t i = 0; i < e->count; i++)
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return 1;
        }

    // Allocate memory for a new entry
    e->count++;
    e->syms = realloc(e->syms, sizeof(char *) * e->count);
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);

    // Copy both K and V into respective lists
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
    e->vals[e->count - 1] = lval_copy(v);

    return 0;
}

int lenv_def(lenv *e, lval *k, lval *v)
{
    // Update E to global environment
    while (e->parent)
        e = e->parent;

    return lenv_put(e, k, v);
}

void lenv_add_builtin(lenv *e, char *id, lbuiltin func)
{
    // Construct lvals from the ID and FUNC
    lval *k = lval_sym(id);
    lval *v = lval_fun(func);

    // Insert both values in respective lists
    lenv_put(e, k, v);

    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv *e)
{
    // Register Builtin List Functions
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "len", builtin_len);

    // Register Variable Functions
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "\\", builtin_lambda);

    // Register Builtin Arithmetic Functions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "pow", builtin_pow);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);
}

/* ------------------------------------ */
/* ---------- LVAL Functions ---------- */
/* ------------------------------------ */

lval *lval_num(long num)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = num;
    return v;
}

lval *lval_err(char *fmt, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    // Create variadic array list and initialize
    va_list va;
    va_start(va, fmt);

    // Allocate 512 bytes for error message, hopefully enough
    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);

    // Reallocate to bytes actually used
    v->err = realloc(v->err, strlen(v->err) + 1);

    va_end(va);

    return v;
}

lval *lval_sym(char *symbol)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(symbol) + 1);
    strcpy(v->sym, symbol);
    return v;
}

lval *lval_sexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_fun(lbuiltin func)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

lval *lval_qexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_lambda(lval *formals, lval *body)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;

    return v;
}

void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        // Nothing additional to free
        break;
    case LVAL_ERR:
        // Free error string allocation
        free(v->err);
        break;
    case LVAL_SYM:
        // Free symbol string allocation
        free(v->sym);
        break;
    case LVAL_FUN:
        if (!v->builtin)
        {
            lenv_del(v->env);
            lval_del(v->formals);
            lval_del(v->body);
        }
        break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        // Call lval_del on all elements within cell
        for (size_t i = 0; i < v->count; i++)
            lval_del(v->cell[i]);
        // Free cell allocation
        free(v->cell);
        break;
    default:
        break;
    }

    // Free the memory on the lval struct itself
    free(v);
}

lval *lval_read_num(mpc_ast_t *t)
{
    // Canary variable for when conversion fails
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_read(mpc_ast_t *t)
{
    // Numbers and Symbols are directly parsed
    if (strstr(t->tag, "number"))
        return lval_read_num(t);

    if (strstr(t->tag, "symbol"))
        return lval_sym(t->contents);

    // Node is root or sexpr
    lval *x = NULL;
    if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr"))
        x = lval_sexpr();

    // Node is qexpr
    if (strstr(t->tag, "qexpr"))
        x = lval_qexpr();

    // Add each valid sub-expression contained in the tree to new lval
    for (size_t i = 0; i < t->children_num; i++)
    {
        if (strcmp(t->children[i]->contents, "(") == 0 ||
            strcmp(t->children[i]->contents, ")") == 0 ||
            strcmp(t->children[i]->contents, "{") == 0 ||
            strcmp(t->children[i]->contents, "}") == 0 ||
            strcmp(t->children[i]->tag, "regex") == 0)
            continue;

        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

lval *lval_add(lval *v, lval *x)
{
    // Increment number of valid expressions
    v->count++;

    // Reallocate more space for valid expressions
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    // Append expression to list
    v->cell[v->count - 1] = x;

    return v;
}

char *ltype_name(int t)
{
    switch (t)
    {
    case LVAL_FUN:
        return "Function";
        break;
    case LVAL_NUM:
        return "Number";
        break;
    case LVAL_ERR:
        return "Error";
        break;
    case LVAL_SYM:
        return "Symbol";
        break;
    case LVAL_SEXPR:
        return "S-Expression";
        break;
    case LVAL_QEXPR:
        return "Q-Expression";
        break;
    default:
        return "Unknown";
        break;
    }
}

lval *lval_pop(lval *v, int i)
{
    // Store lval at position i in cell
    lval *x = v->cell[i];

    // Erase lval at position i in cell by shifting the remaining list towards
    // it
    // ! Very important
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (--v->count - i));

    // Reallocate memory since we decreased pointer memory
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    return x;
}

lval *lval_push(lval *v, lval *x)
{
    // TODO : Understand what is wrong
    // Reallocate for one more [lval *] element
    // v->count++;
    // v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    // Shift every element of the list to the right
    // ! Very important
    // memmove(&v->cell[1], &v->cell[0], sizeof(lval *) * v->count);

    // Directly put x as first element
    // v->cell[0] = x;

    return v;
}

lval *lval_copy(lval *v)
{
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type)
    {
    case LVAL_NUM:
        x->num = v->num;
        break;
    case LVAL_ERR:
        x->err = malloc(sizeof(v->err) + 1);
        strcpy(x->err, v->err);
        break;
    case LVAL_SYM:
        x->sym = malloc(sizeof(v->sym) + 1);
        strcpy(x->sym, v->sym);
        break;
    case LVAL_FUN:
        if (v->builtin)
            x->builtin = v->builtin;
        else
        {
            x->builtin = NULL;
            x->env = lenv_copy(v->env);
            x->formals = lval_copy(v->formals);
            x->body = lval_copy(v->body);
        }
        break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(lval *) * x->count);
        for (size_t i = 0; i < x->count; i++)
            x->cell[i] = lval_copy(v->cell[i]);
    default:
        break;
    }

    return x;
}

lval *lval_take(lval *v, int i)
{
    lval *x = lval_pop(v, i);
    lval_del(v);

    return x;
}

lval *lval_join(lval *x, lval *y)
{
    // Add every element of y to x
    while (y->count)
        x = lval_add(x, lval_pop(y, 0));
    lval_del(y);

    return x;
}

lval *lval_cons(lval *x, lval *y)
{
    // Take last element from y, append it to the start of x
    // TODO : Implement a working [lval_push]
    x = lval_push(x, y);
    lval_del(y);

    return x;
}

lval *lval_call(lenv *e, lval *f, lval *a)
{
    // Builtin functions are called as normal
    if (f->builtin)
        return f->builtin(e, a);

    // Number of parameters, number of arguments
    int expected = f->formals->count;
    int given = a->count;

    while (a->count)
    {
        // Do not allow more arguments than parameters
        if (f->formals->count == 0)
        {
            lval_del(a);
            return lval_err(
                "Function passed too many arguments -- Got %d, Expected %d",
                given, expected);
        }
        // Pop first parameter, pop first argument
        // Bind argument to parameter in function's local environment
        lval *sym = lval_pop(f->formals, 0);
        lval *val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);

        lval_del(sym);
        lval_del(val);
    }
    lval_del(a);

    // Evaluate function if all parameters were filled with arguments
    if (f->formals->count == 0)
    {
        // Add calling environment as parent
        // Evaluate function body
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }
    // Allow partially evaluated function to be bound
    return lval_copy(f);
}

/* --------------------------------------------------------- */
/* ---------- List Manipulation Builtin Functions ---------- */
/* --------------------------------------------------------- */

lval *builtin_head(lenv *e, lval *v)
{
    // Requires [one] [non-empty] [Q-Expression] argument
    LASSERT_NUMARGS("head", v, 1);
    LASSERT_NON_EMPTY("head", v, 0);
    LASSERT_TYPE("head", v, 0, LVAL_QEXPR);

    // Take the first element of the Q-Expression
    // Return first, delete all remaining elements
    lval *x = lval_take(v, 0);
    while (x->count > 1)
        lval_del(lval_pop(x, 1));

    return x;
}

lval *builtin_tail(lenv *e, lval *v)
{
    // Requires [one] [non-empty] [Q-Expression] argument
    LASSERT_NUMARGS("tail", v, 1);
    LASSERT_NON_EMPTY("tail", v, 0);
    LASSERT_TYPE("tail", v, 0, LVAL_QEXPR);

    // Take the first element of the Q-Expression
    // Delete first element, return remaining
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));

    return x;
}

lval *builtin_list(lenv *e, lval *v)
{
    v->type = LVAL_QEXPR;
    return v;
}

lval *builtin_join(lenv *e, lval *v)
{
    // Requires every argument to be a Q-Expression
    for (size_t i = 0; i < v->count; i++)
        LASSERT_TYPE("join", v, i, LVAL_QEXPR);

    lval *x = lval_pop(v, 0);
    while (v->count)
        x = lval_join(x, lval_pop(v, 0));
    lval_del(v);

    return x;
}

lval *builtin_eval(lenv *e, lval *v)
{
    // Requires [one] [Q-Expression] argument
    LASSERT_NUMARGS("eval", v, 1);
    LASSERT_TYPE("eval", v, 0, LVAL_QEXPR);

    lval *x = lval_take(v, 0);
    x->type = LVAL_SEXPR;

    return lval_eval(e, x);
}

lval *builtin_cons(lenv *e, lval *v)
{
    // Requires [two] arguments, first is Q-Expression
    LASSERT_NUMARGS("cons", v, 2);
    LASSERT_TYPE("cons", v, 0, LVAL_QEXPR);

    lval *x = lval_pop(v, 0);
    x = lval_cons(x, lval_pop(v, 0));
    lval_del(v);

    return x;
}

lval *builtin_len(lenv *e, lval *v)
{
    // Requires [one] [Q-Expression] argument
    LASSERT_NUMARGS("len", v, 1);
    LASSERT_TYPE("len", v, 0, LVAL_QEXPR);

    lval *x = lval_take(v, 0);
    long length = x->count;
    lval_del(x);

    return lval_num(length);
}

lval *builtin_init(lenv *e, lval *v)
{
    // Requires [one] [Q-Expression] argument
    LASSERT_NUMARGS("len", v, 1);
    LASSERT_TYPE("len", v, 0, LVAL_QEXPR);

    // Take everything except the last element of the Q-Expression
    // Delete last element, return remaining
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, x->count - 1));

    return x;
}

lval *builtin_def(lenv *e, lval *v)
{
    return builtin_var(e, v, "def");
}

lval *builtin_put(lenv *e, lval *v)
{
    return builtin_var(e, v, "=");
}

lval *builtin_var(lenv *e, lval *v, char *func)
{
    // Ensure first argument is a list of symbols
    LASSERT_TYPE(func, v, 0, LVAL_QEXPR);

    lval *syms = v->cell[0];

    // Ensure all following elements are also symbols
    for (size_t i = 0; i < syms->count; i++)
        LASSERT(v, syms->cell[i]->type == LVAL_SYM,
                "Funtion '%s' cannot define non-symbol -- Got %s, Expected %s",
                func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));

    // Ensure arity between symbols and values
    LASSERT(v, (syms->count == v->count - 1),
            "Function '%s' expects match between number of symbols and values "
            "-- Got %d, Expected %d",
            func, syms->count, v->count - 1);

    // Insert key value pairs
    for (size_t i = 0; i < syms->count; i++)
    {
        if (strcmp(func, "def") == 0)
            lenv_def(e, syms->cell[i], v->cell[i + 1]);
        if (strcmp(func, "=") == 0)
            lenv_put(e, syms->cell[i], v->cell[i + 1]);
    }

    lval_del(v);

    return lval_sexpr();
}

lval *builtin_lambda(lenv *e, lval *v)
{
    // Requires [two] [Q-Expressions]
    LASSERT_NUMARGS("\\", v, 2);
    LASSERT_TYPE("\\", v, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", v, 1, LVAL_QEXPR);

    for (size_t i = 0; i < v->cell[0]->count; i++)
        LASSERT(v, (v->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol -- Got %s, Expected %s",
                ltype_name(v->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));

    // Extract first and second args
    lval *formals = lval_pop(v, 0);
    lval *body = lval_pop(v, 0);
    lval_del(v);

    return lval_lambda(formals, body);
}

/* -------------------------------------------------- */
/* ---------- Arithmetic Builtin Functions ---------- */
/* -------------------------------------------------- */

lval *builtin_op(lenv *e, lval *v, char *op)
{
    // Ensure all arguments are numbers
    for (size_t i = 0; i < v->count; i++)
        LASSERT_TYPE(op, v, i, LVAL_NUM);

    // Attempt to perform unary negation
    lval *x = lval_pop(v, 0);
    if ((strcmp(op, "-") == 0) && v->count == 0)
        x->num = -x->num;

    // Calculate expressions normally
    while (v->count > 0)
    {
        // LVAL_POP decreases v->count
        lval *y = lval_pop(v, 0);

        if (strcmp(op, "+") == 0)
            x->num += y->num;

        if (strcmp(op, "-") == 0)
            x->num -= y->num;

        if (strcmp(op, "*") == 0)
            x->num *= y->num;

        if (strcmp(op, "min") == 0)
            x->num = x->num < y->num ? x->num : y->num;

        if (strcmp(op, "max") == 0)
            x->num = x->num >= y->num ? x->num : y->num;

        if (strcmp(op, "/") == 0)
        {
            if (y->num == 0)
            {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero");
                break;
            }
            x->num /= y->num;
        }

        if (strcmp(op, "%") == 0)
        {
            if (y->num == 0)
            {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by Zero");
                break;
            }
            x->num %= y->num;
        }

        if (strcmp(op, "pow") == 0)
        {
            if (y->num < 0)
            {
                lval_del(x);
                lval_del(y);
                x = lval_err("Negative Exponent");
                break;
            }
            x->num = pow(x->num, y->num);
        }
        lval_del(y);
    }
    lval_del(v);

    return x;
}
lval *builtin_add(lenv *e, lval *v)
{
    return builtin_op(e, v, "+");
}
lval *builtin_sub(lenv *e, lval *v)
{
    return builtin_op(e, v, "-");
}
lval *builtin_mul(lenv *e, lval *v)
{
    return builtin_op(e, v, "*");
}
lval *builtin_div(lenv *e, lval *v)
{
    return builtin_op(e, v, "/");
}
lval *builtin_mod(lenv *e, lval *v)
{
    return builtin_op(e, v, "%");
}
lval *builtin_pow(lenv *e, lval *v)
{
    return builtin_op(e, v, "pow");
}
lval *builtin_min(lenv *e, lval *v)
{
    return builtin_op(e, v, "min");
}
lval *builtin_max(lenv *e, lval *v)
{
    return builtin_op(e, v, "max");
}

/* ---------------------------- */
/* ---------- PRINT  ---------- */
/* ---------------------------- */

void lenv_print_definitions(lenv *e)
{
    for (size_t i = 0; i < e->count; i++)
        printf("%s ", e->syms[i]);
    putchar('\n');
}

void lval_expr_print(lenv *e, lval *v, char open, char close)
{
    putchar(open);
    for (size_t i = 0; i < v->count; i++)
    {
        lval_print(e, v->cell[i]);

        // Avoid trailing space if not end
        if (i != (v->count - 1))
            putchar(' ');
    }
    putchar(close);
}

void lval_print_func(lenv *e, lval *v)
{
    // Returns symbol or error
    lval *tmp = lenv_get_key(e, v);
    lval_print(e, tmp);
    lval_del(tmp);
}

void lval_print(lenv *e, lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        printf("%ld", v->num);
        break;
    case LVAL_ERR:
        printf("Error: %s", v->err);
        break;
    case LVAL_SYM:
        printf("%s", v->sym);
        break;
    case LVAL_FUN:
        if (v->builtin)
            lval_print_func(e, v);
        else
        {
            printf("(\\ ");
            lval_print(e, v->formals);
            putchar(' ');
            lval_print(e, v->body);
            putchar(')');
        }
        break;
    case LVAL_SEXPR:
        lval_expr_print(e, v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_expr_print(e, v, '{', '}');
        break;
    default:
        printf("UNEXPECTED ERROR");
        break;
    }
}

void lval_println(lenv *e, lval *v)
{
    lval_print(e, v);
    putchar('\n');
}

/* --------------------------- */
/* ---------- EVAL  ---------- */
/* --------------------------- */

lval *lval_eval_sexpr(lenv *e, lval *v)
{
    // Evaluale children
    for (size_t i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(e, v->cell[i]);

    // Error checking
    for (size_t i = 0; i < v->count; i++)
        if (v->cell[i]->type == LVAL_ERR)
            return lval_take(v, i);

    // Empty expression
    if (v->count == 0)
        return v;

    // Single expression
    if (v->count == 1)
        return lval_take(v, 0);

    // Ensure first element is a Function
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN)
    {
        lval *err = lval_err(
            "S-Expression starts with incorrect type -- Got %s, Expected %s",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }

    // ! Call BUILTIN FUNCTION STORED IN E to evaluate remaining of V
    lval *result = lval_call(e, f, v);
    lval_del(f);

    return result;
}

lval *lval_eval(lenv *e, lval *v)
{
    if (v->type == LVAL_SYM)
    {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }

    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(e, v);

    return v;
}
