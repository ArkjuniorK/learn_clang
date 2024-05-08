#include <stdio.h>
#include "mpc.h"
#include "lval.h"

#define LASSERT(args, cond, err) \
    if (!(cond))                 \
    {                            \
        lval_del(args);          \
        return lval_err(err);    \
    }

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

// Construct qexpr lval type
lval *lval_qexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->cell = NULL;
    v->count = 0;
    return v;
}

// Increase lval counter and append the given lval it to cell
lval *lval_push(lval *x, lval *y)
{
    x->count++;
    x->cell = realloc(x->cell, sizeof(lval *) * x->count);
    x->cell[x->count - 1] = y;
    return x;
}

// Add y to x at the beginning
lval *lval_unshift(lval *x, lval *y)
{
    x->count++;
    x->cell = realloc(x->cell, sizeof(lval *) * x->count);
    for (int i = (x->count - 1); i > 0; i--)
    {
        x->cell[i] = x->cell[i - 1];
    }

    x->cell[0] = y;
    return x;
}

// Pop specific lval
lval *lval_pop(lval *v, int i)
{
    lval *c = v->cell[i];

    // Shift the memory after the item at "i" over the top
    memmove(&v->cell[i], &v->cell[i + 1],
            sizeof(lval *) * (v->count - i - 1));

    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    return c;
}

// Take specific lval from given index and delete the rest
lval *lval_take(lval *v, int i)
{
    lval *c = lval_pop(v, i);
    lval_del(v);
    return c;
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
        return lval_read_num(t);

    if (strstr(t->tag, "symbol"))
        return lval_sym(t->contents);

    lval *v = NULL;

    if ((strcmp(t->tag, ">")) == 0 || strstr(t->tag, "sexpr"))
        v = lval_sexpr();

    if (strstr(t->tag, "qexpr"))
        v = lval_qexpr();

    for (int i = 0; i < t->children_num; i++)
    {
        if ((strcmp(t->children[i]->contents, "(") == 0) ||
            (strcmp(t->children[i]->contents, ")") == 0) ||
            (strcmp(t->children[i]->contents, "{") == 0) ||
            (strcmp(t->children[i]->contents, "}") == 0) ||
            (strcmp(t->children[i]->tag, "regex") == 0))
            continue;

        v = lval_push(v, lval_read(t->children[i]));
    }

    return v;
}

// Evaluate sexpr lval
lval *lval_eval_sexpr(lval *v)
{
    for (int i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(v->cell[i]);

    for (int i = 0; i < v->count; i++)
    {
        if (v->cell[i]->type == LVAL_ERR)
        {
            return lval_take(v, i);
        }
    }

    if (v->count == 0)
        return v;

    if (v->count == 1)
        return lval_take(v, 0);

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("S-Expressin did not start with Symbol");
    }

    lval *r = builtin(v, f->sym);
    lval_del(f);
    return r;
}

// Evaluate lval
lval *lval_eval(lval *v)
{
    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(v);

    return v;
}

// Join each elements from y to x
lval *lval_join(lval *x, lval *y)
{
    while (y->count)
    {
        x = lval_push(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
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
    case LVAL_QEXPR:
        for (int i = 0; i < v->count; i++)
        {
            lval_del(v->cell[i]);
        }

        free(v->cell);
        break;
    }

    free(v);
}

// Print the exp type of lval
void lval_expr_print(lval *v, char open, char close)
{
    putchar(open);

    for (int i = 0; i < v->count; i++)
    {
        lval_print(v->cell[i]);

        // Do not print trailing space at last element
        if (i != (v->count - 1))
        {
            putchar(' ');
        }
    }

    putchar(close);
}

// Print the `lval`
void lval_print(lval *v)
{
    switch (v->type)
    {
    case LVAL_ERR:
        printf("Error: %s", v->err);
        break;

    case LVAL_NUM:
        printf("%li", v->num);
        break;

    case LVAL_SYM:
        printf("%s", v->sym);
        break;

    case LVAL_SEXPR:
        lval_expr_print(v, '(', ')');
        break;

    case LVAL_QEXPR:
        lval_expr_print(v, '{', '}');
        break;
    }
}

// Print lval value by newline
void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
}

// Take the first element from qexpr and remove the rest
lval *builtin_head(lval *a)
{
    LASSERT(a, a->count == 1,
            "Function 'head' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head' passed invalid type!");
    LASSERT(a, a->cell[0]->count != 0,
            "Function 'head' passed {}!");

    lval *f = lval_take(a, 0);
    while (f->count > 1)
    {
        lval_del(lval_pop(f, 1));
    }

    return f;
}

// Take and remove the first element from qexpr
lval *builtin_tail(lval *a)
{
    LASSERT(a, a->count == 1,
            "Function 'tail' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'tail' passed invalid type!");
    LASSERT(a, a->cell[0]->count != 0,
            "Function 'tail' passed {}!");

    lval *f = lval_take(a, 0);
    lval_del(lval_pop(f, 0));
    return f;
}

lval *builtin_list(lval *a)
{
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lval *a)
{
    LASSERT(a, a->count == 1,
            "Function 'eval' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed invalid type!");

    lval *f = lval_take(a, 0);
    f->type = LVAL_SEXPR;
    return lval_eval(f);
}

lval *builtin_join(lval *a)
{
    for (int i = 0; i < a->count; i++)
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                "Function 'join' passed incorrect type!");

    lval *f = lval_pop(a, 0);

    while (a->count)
    {
        f = lval_join(f, lval_pop(a, 0));
    }

    lval_del(a);
    return f;
}

lval *builtin_cons(lval *a)
{
    LASSERT(a, a->count == 2,
            "Function 'cons' passed too many arguments");
    LASSERT(a, a->cell[0]->type == LVAL_NUM,
            "Function 'cons' should pass number as first argument");
    LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
            "Function 'cons' should pass Q-Expr as second argument");

    lval *f = lval_pop(a, 0);
    f = lval_unshift(lval_pop(a, 0), f);

    lval_del(a);
    return f;
}

lval *builtin_len(lval *a)
{
    return lval_num(a->count);
}

lval *builtin_init(lval *a)
{
    LASSERT(a, a->count > 1,
            "Function init passed to few arguments");

    lval_pop(a, 0);
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_op(lval *a, char *op)
{
    for (int i = 0; i < a->count; i++)
    {
        if (a->cell[i]->type != LVAL_NUM)
        {
            lval_del(a);
            return lval_err("Cannot operate on non-number");
        }
    }

    lval *f = lval_pop(a, 0);
    if ((strcmp(op, "-") == 0) && a->count == 0)
    {
        f->num = -f->num;
    }

    while (a->count > 0)
    {
        lval *n = lval_pop(a, 0);

        if (strcmp(op, "+") == 0)
            f->num += n->num;

        if (strcmp(op, "-") == 0)
            f->num -= n->num;

        if (strcmp(op, "*") == 0)
            f->num *= n->num;

        if (strcmp(op, "/") == 0)
        {
            if (n->num == 0)
            {
                lval_del(f);
                lval_del(n);
                f = lval_err("Division by zero!");
                break;
            }

            f->num /= n->num;
        }

        if (strcmp(op, "^") == 0)
            f->num = pow(f->num, n->num);

        if (strcmp(op, "%") == 0)
        {
            if (n->num == 0)
            {
                lval_del(f);
                lval_del(n);
                f = lval_err("Comparable by zero!");
                break;
            }

            f->num = f->num % n->num;
        }

        lval_del(n);
    }

    lval_del(a);
    return f;
}

lval *builtin(lval *a, char *fn)
{
    if (strcmp("head", fn) == 0)
        return builtin_head(a);

    if (strcmp("tail", fn) == 0)
        return builtin_tail(a);

    if (strcmp("list", fn) == 0)
        return builtin_list(a);

    if (strcmp("join", fn) == 0)
        return builtin_join(a);

    if (strcmp("cons", fn) == 0)
        return builtin_cons(a);

    if (strcmp("len", fn) == 0)
        return builtin_len(a);

    if (strcmp("init", fn) == 0)
        return builtin_init(a);

    if (strcmp("eval", fn) == 0)
        return builtin_eval(a);

    if (strstr("+-/*^%", fn))
        return builtin_op(a, fn);

    lval_del(a);
    return lval_err("Unknown function!");
}