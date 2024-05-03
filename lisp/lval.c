#include <stdio.h>
#include <string.h>
#include "mpc.h"
#include "lval.h"

// Construct number lval type
lval *lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct error lval type
lval *lval_err(char *m)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// Construct symbol lval type
lval *lval_sym(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// Construct sexpr lval type
lval *lval_sexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->cell = NULL;
    v->count = 0;
    return v;
}

// Increase lval counter and append the given lval it to cell
lval *lval_add(lval *v, lval *x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

// Read number type content and construct
lval *lval_read_num(mpc_ast_t *t)
{
    errno = 0;
    long n = strtol(t->contents, NULL, 0);
    return errno != ERANGE ? lval_num(n) : lval_err("invalid number");
}

// Read the tree
lval *lval_read(mpc_ast_t *t)
{
    if (strstr(t->tag, "number"))
    {
        return lval_read_num(t);
    }

    if (strstr(t->tag, "symbol"))
    {
        return lval_sym(t->contents);
    }

    // If root (>) or sexpr create an empty list
    lval *l = NULL;
    if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr"))
    {
        l = lval_sexpr();
    }

    for (int i = 0; i < t->children_num; i++)
    {
        if (strcmp(t->children[i]->contents, "(") == 0 ||
            strcmp(t->children[i]->contents, ")") == 0 ||
            strcmp(t->children[i]->tag, "regex") == 0)
        {
            continue;
        }

        l = lval_add(l, lval_read(t->children[i]));
    }

    return l;
}

// Delete lval from memory
void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        break;

    case LVAL_ERR:
        free(v->err);
        break;
    case LVAL_SYM:
        free(v->sym);
        break;

    // Free the children first then release the root
    case LVAL_SEXPR:
        for (int i = 0; i < v->count; i++)
        {
            lval_del(v->cell[i]);
        }

        free(v->cell);
    }

    free(v);
}

// Print the `lval`
void lval_print(lval v)
{
    switch (v.type)
    {
    case LVAL_ERR:
        // switch (v.err)
        // {
        // case LERR_BAD_OP:
        //     printf("Error: Invalid operator!");
        //     break;

        // case LERR_BAD_NUM:
        //     printf("Error: Invalid number!");
        //     break;

        // case LERR_DIV_ZERO:
        //     printf("Error: Division by zero!");
        //     break;
        // }
        break;

    case LVAL_NUM:
        printf("%li", v.num);
        break;
    }
}

// Print lval value by newline
void lval_println(lval v)
{
    lval_print(v);
    putchar('\n');
}