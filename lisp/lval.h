#ifndef lval_h
#define lval_h

// Lisp Value struct
typedef struct lval
{
    int type;
    long num;

    char *err;
    char *sym;

    int count;
    struct lval **cell;
} lval;

// Enumeration of posibble Lisp Value types
enum
{
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_SEXPR,
};

lval lval_err(int x);
lval lval_num(long x);

void lval_print(lval v);
void lval_println(lval v);

#endif