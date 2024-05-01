#ifndef lval_h
#define lval_h

// Lisp Value struct
typedef struct
{
    int type;
    long num;
    int err;
} lval;

// Enumeration of posibble Lisp Value types
enum
{
    LVAL_NUM,
    LVAL_ERR
};

// Enumeration of posibble error types of Lisp Value
enum
{
    LERR_BAD_OP,
    LERR_BAD_NUM,
    LERR_DIV_ZERO,
};

lval lval_err(int x);
lval lval_num(long x);

void lval_print(lval v);
void lval_println(lval v);

#endif