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
    LVAL_QEXPR,
};

lval *lval_num(long n);
lval *lval_err(char *e);
lval *lval_sym(char *s);
lval *lval_sexpr();
lval *lval_qexpr();

lval *lval_eval(lval *v);
lval *lval_read(mpc_ast_t *t);

lval *builtin(lval *a, char *fn);

void lval_del(lval *v);

void lval_print(lval *v);
void lval_println(lval *v);

#endif