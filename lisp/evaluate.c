#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mpc.h"
#include "lval.h"

// Use operator string to see which operator being use
lval eval_op(lval x, char *op, lval y)
{
    // If either value is an error return it
    if (x.type == LVAL_ERR)
    {
        return x;
    }

    if (y.type == LVAL_ERR)
    {
        return y;
    }

    // otherwise do match on the number values
    if (strcmp(op, "+") == 0)
    {
        return lval_num(x.num + y.num);
    }

    if (strcmp(op, "-") == 0)
    {
        return lval_num(x.num - y.num);
    }

    if (strcmp(op, "*") == 0)
    {
        return lval_num(x.num * y.num);
    }

    if (strcmp(op, "/") == 0)
    {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    if (strcmp(op, "^") == 0)
    {
        return lval_num(pow(x.num, y.num));
    }

    if (strcmp(op, "%") == 0)
    {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);
    }

    if (strcmp(op, "min") == 0)
    {
        return lval_num(fmin(x.num, y.num));
    }

    if (strcmp(op, "max") == 0)
    {
        return lval_num(fmax(x.num, y.num));
    }

    return lval_err(LERR_BAD_OP);
}

// Evaluate the user input and return the result
lval eval(mpc_ast_t *t)
{
    // If tagged as number return directly
    if (strstr(t->tag, "number"))
    {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // Operator is always second child
    char *op = t->children[1]->contents;

    // Store the third child to x
    lval v = eval(t->children[2]);

    // If operator is '-' with only one parameter then negate the value
    if (strcmp(op, "-") == 0 && strstr(t->children[3]->tag, "regex"))
    {
        v.num = -v.num;
        return v;
    }

    // Iterate remaining child and combine it
    int i = 3;
    while (strstr(t->children[i]->tag, "expr"))
    {
        v = eval_op(v, op, eval(t->children[i]));
        i++;
    }

    return v;
}