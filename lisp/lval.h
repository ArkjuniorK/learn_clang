#ifndef lval_h
#define lval_h

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum
{
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_FUNC,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_NONE,
};

typedef lval *(*lbuiltin)(lenv *, lval *);

struct lval
{
    int type;
    long num;

    char *err;
    char *sym;
    lbuiltin func;

    int count;
    struct lval **cell;
};

struct lenv
{
    int count;
    char **syms;
    lval **vals;
};

lenv *lenv_new(void);
lval *lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_del(lenv *e);
void lenv_add_builtins(lenv *e);

lval *lval_num(long n);
lval *lval_sym(char *s);
lval *lval_err(char *fmt, ...);
lval *lval_sexpr();
lval *lval_qexpr();

lval *lval_eval(lenv *e, lval *v);
lval *lval_read(mpc_ast_t *t);

void lval_del(lval *v);
void lval_print(lval *v);
void lval_println(lval *v);

#endif