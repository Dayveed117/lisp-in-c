#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enables editing lines in cli
#include <editline/history.h>
#include <editline/readline.h>

// Parser Combinator Library
#include "eval.h"
#include "lib.h"
#include "mpc.h"

// Forward declare parsers
mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispy;

void run_prompt(lenv *e, mpc_parser_t *parser);
void run_interpreter(lenv *e, int argc, char **argv);

int main(int argc, char **argv)
{
    // Defining the Grammar for Lisp-in-C
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, "                                            \
        number   : /-?[0-9]+[.]?[0-9]*/;                                \
        symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^%|]+/;                 \
        string   : /\"(\\\\.|[^\"])*\"/;                                \
        comment  : /;[^\\r\\n]*/;                                       \
        sexpr    : '(' <expr>* ')';                                     \
        qexpr    : '{' <expr>* '}';                                     \
        expr     : <number> | <symbol> |  <string> | <sexpr> | <qexpr>; \
        lispy    : /^/ <expr>* /$/;                                     \
    ",
              Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    // Create environment for variables and functions
    // Register builtin functions
    lenv *env = lenv_new();
    lenv_add_builtins(env);

    if (argc <= 1)
    {
        run_prompt(env, Lispy);
    }
    else
    {
        run_interpreter(env, argc, argv);
    }

    lenv_del(env);
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}

// ! Requires Lispy parser
lval *builtin_load(lenv *e, lval *v)
{
    LASSERT_NUMARGS("load", v, 1);
    LASSERT_TYPE("load", v, 0, LVAL_STR);

    // Parse file given by arg
    mpc_result_t r;
    if (mpc_parse_contents(v->cell[0]->str, Lispy, &r))
    {
        // Read contents
        lval *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        // Evaluate each expression
        while (expr->count)
        {
            lval *x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR)
                lval_println(e, x);
            lval_del(x);
        }

        lval_del(expr);
        lval_del(v);

        // Return empty expression if success
        return lval_sexpr();
    }
    // Error when parsing file
    else
    {
        // Extract parsing error
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        // Create error message with it
        lval *err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(v);

        // Return error on failure
        return err;
    }
}

void run_prompt(lenv *e, mpc_parser_t *parser)
{
    puts("Lispy Version 0.8");
    puts("Press Ctrl+c or type \"exit\" to exit\n");

    while (1)
    {
        char *buf = readline("lispy> ");
        add_history(buf);

        if (strncmp(buf, "exit", 4) == 0)
            break;

        if (strncmp(buf, "symbols", 4) == 0)
        {
            lenv_print_definitions(e);
            continue;
        }

        mpc_result_t r;
        if (mpc_parse("<stdin>", buf, parser, &r))
        {
            // Successful parse
            lval *x = lval_eval(e, lval_read(r.output));
            lval_println(e, x);
            lval_del(x);

            // printf("Nº of nodes: %d\n", number_of_nodes(r.output));
            // printf("Nº of leaves: %d\n", number_of_leaves(r.output));
            // printf("Nº of branches: %d\n\n",
            // number_of_branches(r.output)); mpc_ast_print(r.output);

            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(buf);
    }
}

void run_interpreter(lenv *e, int argc, char **argv)
{
    // Loop through each main argument, filenames given
    for (size_t i = 1; i < argc; i++)
    {
        // Build a lval with all the file's expressions
        lval *args = lval_add(lval_sexpr(), lval_str(argv[i]));

        // Load will parse and evaluate each expression
        lval *x = builtin_load(e, args);

        if (x->type == LVAL_ERR)
            lval_println(e, x);
        lval_del(x);
    }
}
