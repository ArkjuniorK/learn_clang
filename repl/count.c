#include "mpc.h"

int count_leaves(mpc_ast_t *t)
{
    if (strstr(t->tag, "number") || strstr(t->tag, "operator"))
    {
        return 1;
    }

    int total = 0;
    if (t->children_num != 0)
    {
        for (int i = 0; i < t->children_num; i++)
        {
            total = total + count_leaves(t->children[i]);
        }
    }

    return total;
}

int count_branches(mpc_ast_t *t)
{
    if (strstr(t->tag, "expr") && t->children_num != 0)
    {
        return 1;
    }

    int total = 0;
    for (int i = 0; i < t->children_num; i++)
    {
        total = total + count_branches(t->children[i]);
    }

    return total;
}

int count_max_child(mpc_ast_t *t)
{
    if (strstr(t->tag, "expr") && t->children_num != 0)
    {
        return t->children_num;
    }

    int total = 0;
    for (int i = 0; i < t->children_num; i++)
    {
        int curr = count_max_child(t->children[i]);
        if (total < curr)
        {
            total = curr;
        }
    }

    return total;
}