#include "mpc.h"

int main(int argc, char *argv[])
{
    // Building a parser 'Adjective' to recognize descriptions
    mpc_parser_t *Ajective = mpc_or(4, mpc_sym("wow"), mpc_sym("many"),
                                    mpc_sym("so"), mpc_sym("such"));

    // Building a parser 'Noun' to recognize things
    mpc_parser_t *Noun =
        mpc_or(5, mpc_sym("lisp"), mpc_sym("language"), mpc_sym("book"),
               mpc_sym("build"), mpc_sym("c"));

    mpc_parser_t *Phrase = mpc_and(2, mpcf_strfold, Ajective, Noun, free);

    mpc_parser_t *Doge = mpc_many(mpcf_strfold, Phrase);

    mpc_delete(Doge);

    return 0;
}
