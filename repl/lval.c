#include <stdio.h>
#include "lval.h"

// Create number lval type
lval lval_num(long x)
{
    lval v;
    v.num = x;
    v.type = LVAL_NUM;

    return v;
}

// Create error lval type
lval lval_err(int x)
{
    lval v;
    v.err = x;
    v.type = LVAL_ERR;

    return v;
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