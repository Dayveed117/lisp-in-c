#include "lib.h"

void print_child_info(mpc_ast_t *t)
{
    printf("Tag: %s\n", t->tag);
    printf("Contents: %s\n", t->contents);
    printf("Number of Children: %d\n\n", t->children_num);

    for (size_t i = 0; i < t->children_num; i++)
    {
        print_child_info(t->children[i]);
    }
}

int number_of_nodes(mpc_ast_t *t)
{
    if (t->children_num <= 0)
        return 1;

    int total = 0;
    for (size_t i = 0; i < t->children_num; i++)
    {
        total += number_of_nodes(t->children[i]);
    }
    return total;
}

int number_of_leaves(mpc_ast_t *t)
{
    if (strstr(t->tag, "number") || strstr(t->tag, "operator"))
        return 1;

    int total = 0;
    for (size_t i = 0; i < t->children_num; i++)
    {
        total += number_of_leaves(t->children[i]);
    }

    return total;
}

int number_of_branches(mpc_ast_t *t)
{
    int total = 1;
    for (size_t i = 0; i < t->children_num; i++)
    {
        if (strstr(t->children[i]->tag, ">"))
            total += number_of_branches(t->children[i]);
    }

    return total;
}

int biggest_children_branch(mpc_ast_t *t)
{
    if (t->children_num <= 0)
        return 0;

    int max = t->children_num;
    for (size_t i = 0; i < t->children_num; i++)
    {
        int cur = biggest_children_branch(t->children[i]);
        max = cur > max ? cur : max;
    }

    return max;
}
