#include <stdio.h>
#include <string.h>
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
        switch (v.err)
        {
        case LERR_BAD_OP:
            printf("Error: Invalid operator!");
            break;

        case LERR_BAD_NUM:
            printf("Error: Invalid number!");
            break;

        case LERR_DIV_ZERO:
            printf("Error: Division by zero!");
            break;
        }
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