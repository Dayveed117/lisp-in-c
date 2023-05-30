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
    // Defining the Grammar for Polish Notation language
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, "                 \
        number   : /-?[0-9]+[.]?[0-9]*/;           \
        symbol   : '+' | '-' | '*' | '/'           \
                 | '%' | '^' | '<' | '>';          \
        sexpr    : '(' <expr>* ')';                \
        expr     : <number> | <symbol> | <sexpr>;  \
        lispy    : /^/ <expr>* /$/;                \
    ",
              Number, Symbol, Sexpr, Expr, Lispy);

    puts("Lispy Version 0.3");
    puts("Press Ctrl+c or type \"exit\" to exit\n");

    while (1)
    {
        char *buf = readline("lispy> ");
        add_history(buf);

        if (strncmp(buf, "exit", 4) == 0)
            break;

        mpc_result_t r;
        if (mpc_parse("<stdin>", buf, Lispy, &r))
        {
            // Successful parse
            lval *x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);

            // printf("Nº of nodes: %d\n", number_of_nodes(a));
            // printf("Nº of leaves: %d\n", number_of_leaves(a));
            // printf("Nº of branches: %d\n\n", number_of_branches(a));

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

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
