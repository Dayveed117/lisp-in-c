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

int main(int argc, char **argv)
{
    // Defining the Grammar for Lisp-in-C
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    const char *language = "                                      \
        number   : /-?[0-9]+[.]?[0-9]*/;                    \
        symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^%]+/;      \
        sexpr    : '(' <expr>* ')';                         \
        qexpr    : '{' <expr>* '}';                         \
        expr     : <number> | <symbol> | <sexpr> | <qexpr>; \
        lispy    : /^/ <expr>* /$/;                         \
    ";

    mpca_lang(MPCA_LANG_DEFAULT, language, Number, Symbol, Sexpr, Qexpr, Expr,
              Lispy);

    puts("Lispy Version 0.5");
    puts("Press Ctrl+c or type \"exit\" to exit\n");

    // Create environment for variables and functions
    // Register builtin functions
    lenv *env = lenv_new();
    lenv_add_builtins(env);

    while (1)
    {
        char *buf = readline("lispy> ");
        add_history(buf);

        if (strncmp(buf, "exit", 4) == 0)
            break;

        if (strncmp(buf, "symbols", 4) == 0)
        {
            lenv_print_definitions(env);
            continue;
        }

        mpc_result_t r;
        if (mpc_parse("<stdin>", buf, Lispy, &r))
        {
            // Successful parse
            lval *x = lval_eval(env, lval_read(r.output));
            lval_println(env, x);
            lval_del(x);

            // printf("Nº of nodes: %d\n", number_of_nodes(r.output));
            // printf("Nº of leaves: %d\n", number_of_leaves(r.output));
            // printf("Nº of branches: %d\n\n", number_of_branches(r.output));
            // mpc_ast_print(r.output);

            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(buf);
    }

    lenv_del(env);
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
