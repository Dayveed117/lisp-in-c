#ifndef lib_h
#define lib_h

#include "mpc.h"

void print_child_info(mpc_ast_t *t);
int number_of_nodes(mpc_ast_t *t);
int number_of_leaves(mpc_ast_t *t);
int number_of_branches(mpc_ast_t *t);
int biggest_children_branch(mpc_ast_t *t);

#endif
