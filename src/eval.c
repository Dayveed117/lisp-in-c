#include "eval.h"

lval *lval_num(long num)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = num;
    return v;
}

lval *lval_err(char *message)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(message) + 1);
    strcpy(v->err, message);
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

lval *lval_qexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
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

void lval_expr_print(lval *v, char open, char close)
{
    putchar(open);
    for (size_t i = 0; i < v->count; i++)
    {
        lval_print(v->cell[i]);

        // Avoid trailing space if not end
        if (i != (v->count - 1))
            putchar(' ');
    }

    putchar(close);
}

void lval_print(lval *v)
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
    case LVAL_SEXPR:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_expr_print(v, '{', '}');
        break;
    default:
        printf("UNEXPECTED ERROR");
        break;
    }
}

void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
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

lval *builtin_head(lval *v)
{
    LASSERT(v, v->count != 1, "Function 'head' expects only 1 argument");
    LASSERT(v, v->cell[0]->type != LVAL_QEXPR,
            "Function 'head' expects a Q-Expression");
    LASSERT(v, v->cell[0]->count == 0,
            "Function 'head' expects non-empty Q-Expression");

    // Take the first element of the Q-Expression
    // Return first, delete all remaining elements
    lval *x = lval_take(v, 0);
    while (x->count > 1)
        lval_del(lval_pop(x, 1));

    return x;
}

lval *builtin_tail(lval *v)
{
    LASSERT(v, v->count != 1, "Function 'tail' expects only 1 argument");
    LASSERT(v, v->cell[0]->type != LVAL_QEXPR,
            "Function 'tail' expects a Q-Expression");
    LASSERT(v, v->cell[0]->count == 0,
            "Function 'tail' expects non-empty Q-Expression");

    // Take the first element of the Q-Expression
    // Delete first element, return remaining
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));

    return x;
}

lval *builtin_list(lval *v)
{
    v->type = LVAL_QEXPR;
    return v;
}

lval *builtin_join(lval *v)
{
    for (size_t i = 0; i < v->count; i++)
        LASSERT(v, v->cell[i]->type != LVAL_QEXPR,
                "Function 'join' only accepts Q-Expressions");

    lval *x = lval_pop(v, 0);
    while (v->count)
        x = lval_join(x, lval_pop(v, 0));
    lval_del(v);

    return x;
}

lval *builtin_eval(lval *v)
{
    LASSERT(v, v->count != 1, "Function 'eval' expects only 1 argument");
    LASSERT(v, v->cell[0]->type != LVAL_QEXPR,
            "Function 'eval' expects a Q-Expression");

    lval *x = lval_take(v, 0);
    x->type = LVAL_SEXPR;

    return lval_eval(x);
}

lval *builtin_op(lval *v, char *op)
{
    // Ensure all arguments are numbers
    for (size_t i = 0; i < v->count; i++)
    {
        if (v->cell[i]->type != LVAL_NUM)
        {
            lval_del(v);
            return lval_err("Cannot operate on non-number");
        }
    }

    // Attempt to perform unary negation
    lval *x = lval_pop(v, 0);
    if ((strcmp(op, "-") == 0) && v->count == 0)
        x->num = -x->num;

    // Calculate expressions normally
    while (v->count > 0)
    {
        // [lval_pop] decreases [count]
        lval *y = lval_pop(v, 0);

        if (strcmp(op, "+") == 0)
            x->num += y->num;

        if (strcmp(op, "-") == 0)
            x->num -= y->num;

        if (strcmp(op, "*") == 0)
            x->num *= y->num;

        if (strcmp(op, "<") == 0)
            x->num = x->num < y->num ? x->num : y->num;

        if (strcmp(op, ">") == 0)
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

        if (strcmp(op, "^") == 0)
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

lval *builtin(lval *v, char *fun)
{
    if (strcmp(fun, "list") == 0)
        return builtin_list(v);
    if (strcmp(fun, "head") == 0)
        return builtin_head(v);
    if (strcmp(fun, "tail") == 0)
        return builtin_tail(v);
    if (strcmp(fun, "join") == 0)
        return builtin_join(v);
    if (strcmp(fun, "eval") == 0)
        return builtin_eval(v);
    if (strstr("+-*/%^><", fun))
        return builtin_op(v, fun);

    // Function should return in one of the cases above
    // Otherwise, unknown operator
    lval_del(v);

    return lval_err("Unknown Function");
}

lval *lval_eval_sexpr(lval *v)
{
    // Evaluale children
    for (size_t i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(v->cell[i]);

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

    // Ensure first element is symbol
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("S-Expression does not start with Symbol");
    }

    // Evaluate expressions with builtin functions
    lval *result = builtin(v, f->sym);
    lval_del(f);

    return result;
}

lval *lval_eval(lval *v)
{
    // Top level eval for sexpr
    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(v);
    return v;
}
