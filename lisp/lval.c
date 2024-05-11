#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include "lval.h"

/**
 * ------
 * Macros
 * ------
 */

#define LASSERT(args, cond, fmt, ...)             \
    if (!(cond))                                  \
    {                                             \
        lval *err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args);                           \
        return err;                               \
    }

#define LASSERT_TYPE(func, args, index, expect)                     \
    LASSERT(args, args->cell[index]->type == expect,                \
            "Function '%s' passed incorrect type for argument %i. " \
            "Got %s, Expected %s.",                                 \
            func, index,                                            \
            ltype_name(args->cell[index]->type),                    \
            ltype_name(expect))

#define LASSERT_COUNT(func, args, num)                        \
    LASSERT(args, args->count == num,                         \
            "Function '%s' passed %i arguments. Expected %i", \
            func, a->count, num)

#define LASSERT_NOT_EMPTY(func, args, index)           \
    LASSERT(args, args->cell[index]->count != 0,       \
            "Function '%s' passed {} for argument %i", \
            func, index)

/**
 * -----------------
 * Utility functions
 * -----------------
 */

int lfunc_args(const char *name)
{
    if (strcmp(name, "show") == 0 ||
        strcmp(name, "exit") == 0)
        return 1;

    return 0;
}

char *ltype_name(int t)
{
    switch (t)
    {
    case LVAL_NUM:
        return "Number";
    case LVAL_ERR:
        return "Error";
    case LVAL_SYM:
        return "Symbol";
    case LVAL_FUNC:
        return "Function";
    case LVAL_SEXPR:
        return "S-Experssion";
    case LVAL_QEXPR:
        return "Q-Experssion";
    default:
        return "Unknown";
    }
}

/**
 * -----------------------------------------
 * Defined below is all lval based functions
 * for various purpuses sush as constructing,
 * reading, evaluating, etc.
 * -----------------------------------------
 */

// Construct number lval type
lval *lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct error lval type
lval *lval_err(char *fmt, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);

    v->err = realloc(v->err, strlen(v->err) + 1);

    va_end(va);
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

lval *lval_func(char *name, lbuiltin func)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUNC;
    v->func = func;
    v->sym = malloc(strlen(name) + 1);
    strcpy(v->sym, name);
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

// Constuct noen lval type
lval *lval_none(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NONE;
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

// Pop specific lval from the list
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

// Copy lval primarily useful for put things in or out from environment
lval *lval_copy(lval *a)
{
    lval *v = malloc(sizeof(lval));
    v->type = a->type;

    switch (a->type)
    {
    // For num and func copy the value directly
    case LVAL_NUM:
        v->num = a->num;
        break;
    case LVAL_FUNC:
        v->func = a->func;
        v->sym = malloc(strlen(a->sym) + 1);
        strcpy(v->sym, a->sym);
        break;

    // For err and sym which represent by string,
    // copy the value using malloc and strcpy
    case LVAL_ERR:
        v->err = malloc(strlen(a->err) + 1);
        strcpy(v->err, a->err);
        break;
    case LVAL_SYM:
        v->sym = malloc(strlen(a->sym) + 1);
        strcpy(v->sym, a->sym);
        break;

    // Copy list by copying each sub-experssion
    // or query-expression
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        v->count = a->count;
        v->cell = malloc(sizeof(lval *) * v->count);
        for (int i = 0; i < v->count; i++)
        {
            v->cell[i] = lval_copy(a->cell[i]);
        }
        break;
    }

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
lval *lval_eval_sexpr(lenv *e, lval *v)
{
    for (int i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(e, v->cell[i]);

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
    {
        if (lfunc_args(v->cell[0]->sym) == 0 &&
            v->cell[0]->type != LVAL_FUNC)
            return lval_take(v, 0);

        lval *f = lval_pop(v, 0);
        lval *r = f->func(e, v);
        lval_del(f);
        return r;
    }

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUNC)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("First element is not a function");
    }

    lval *r = f->func(e, v);
    lval_del(f);
    return r;
}

lval *lval_eval(lenv *e, lval *v)
{
    if (v->type == LVAL_SEXPR)
    {
        return lval_eval_sexpr(e, v);
    }

    if (v->type == LVAL_SYM)
    {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }

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
    case LVAL_FUNC:
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
    case LVAL_NONE:
        printf("\033[A");
        break;

    case LVAL_ERR:
        printf("Error: %s", v->err);
        break;

    case LVAL_NUM:
        printf("%li", v->num);
        break;

    case LVAL_SYM:
    case LVAL_FUNC:
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

/**
 * --------------------------------------------
 * Defined below is all builtin based functions
 * --------------------------------------------
 */

// Take the first element from qexpr and remove the rest
lval *builtin_head(lenv *e, lval *a)
{
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_COUNT("head", a, 1);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval *f = lval_take(a, 0);
    while (f->count > 1)
    {
        lval_del(lval_pop(f, 1));
    }

    return f;
}

// Take and remove the first element from qexpr
lval *builtin_tail(lenv *e, lval *a)
{
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_COUNT("tail", a, 1);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval *f = lval_take(a, 0);
    lval_del(lval_pop(f, 0));
    return f;
}

lval *builtin_list(lenv *e, lval *a)
{
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a)
{
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
    LASSERT_COUNT("eval", a, 1);
    LASSERT_NOT_EMPTY("eval", a, 0);

    lval *f = lval_take(a, 0);
    f->type = LVAL_SEXPR;
    return lval_eval(e, f);
}

// Join two or more q-expr
lval *builtin_join(lenv *e, lval *a)
{
    LASSERT_COUNT("join", a, 2);

    for (int i = 0; i < a->count; i++)
    {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
        LASSERT_NOT_EMPTY("eval", a, i);
    }

    lval *f = lval_pop(a, 0);

    while (a->count)
    {
        f = lval_join(f, lval_pop(a, 0));
    }

    lval_del(a);
    return f;
}

// Append the given argument to q-expr and
// placed it at the first element
lval *builtin_cons(lenv *e, lval *a)
{
    LASSERT_COUNT("cons", a, 2);
    LASSERT_TYPE("cons", a, 0, LVAL_NUM);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    lval *f = lval_pop(a, 0);
    f = lval_unshift(lval_pop(a, 0), f);

    lval_del(a);
    return f;
}

lval *builtin_len(lenv *e, lval *a)
{
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
    LASSERT_COUNT("len", a, 1);

    return lval_num(a->cell[0]->count);
}

// Display the first item of q-expr
lval *builtin_init(lenv *e, lval *a)
{
    LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
    LASSERT_COUNT("init", a, 1);
    LASSERT_NOT_EMPTY("init", a, 0);

    lval *f = lval_take(a, 0);
    lval_del(lval_pop(f, f->count - 1));

    return f;
}

lval *builtin_def(lenv *e, lval *a)
{
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

    lval *syms = a->cell[0];
    for (int i = 0; i < syms->count; i++)
    {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "Function 'def' cannot define non-symbol");
    }

    LASSERT(a, syms->count == a->count - 1,
            "Function 'def' cannot define incorrect "
            "number of values to symbols");

    for (int i = 0; i < syms->count; i++)
    {
        lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    lval_del(a);
    return lval_sexpr();
}

lval *builtin_show(lenv *e, lval *a)
{
    for (int i = 0; i < e->count; i++)
    {
        if (e->vals[i]->type == LVAL_NUM)
            printf("%s\n", e->syms[i]);
    }

    lval_del(a);
    return lval_none();
}

lval *builtin_exit(lenv *e, lval *a)
{
    LASSERT(a, a->count < 1,
            "Function 'exit' passed too many arguments. "
            "Got %i, Expected %i",
            a->count, 0);

    lenv_del(e);
    lval_del(a);

    exit(EXIT_SUCCESS);
}

lval *builtin_op(lenv *e, lval *a, char *op)
{
    for (int i = 0; i < a->count; i++)
    {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
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

lval *builtin_add(lenv *e, lval *a)
{
    return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a)
{
    return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a)
{
    return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a)
{
    return builtin_op(e, a, "/");
}

lval *builtin_pow(lenv *e, lval *a)
{
    return builtin_op(e, a, "^");
}

lval *builtin_dif(lenv *e, lval *a)
{
    return builtin_op(e, a, "%");
}

/**
 * -----------------------------------------
 * Defined below is all lenv based functions
 * -----------------------------------------
 */

// Construct empty lenv
lenv *lenv_new(void)
{
    lenv *e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

// Get a copy of value of lenv which match the key
lval *lenv_get(lenv *e, lval *k)
{
    for (int i = 0; i < e->count; i++)
    {
        if (strcmp(e->syms[i], k->sym) == 0)
            return lval_copy(e->vals[i]);
    }

    return lval_err("Unbound symbol '%s'", k->sym);
}

// Set or update a value of lenv
void lenv_put(lenv *e, lval *k, lval *v)
{
    // See if a key already exists in environment
    // If it is exists, then replace the current value
    // of the key with the one that user provide
    for (int i = 0; i < e->count; i++)
    {
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    // Otherwise allocate space for new entires
    e->count++;
    e->syms = realloc(e->syms, sizeof(char *) * e->count);
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);

    // Copy the contents to lenv
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

// Delete lenv and its children
void lenv_del(lenv *e)
{
    for (int i = 0; i < e->count; i++)
    {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func)
{
    lval *k = lval_sym(name);
    lval *v = lval_func(name, func);
    lenv_put(e, k, v);
    free(k);
    free(v);
}

// Register all builtin functions
void lenv_add_builtins(lenv *e)
{
    // Variable functions
    lenv_add_builtin(e, "def", builtin_def);

    // List functions
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "show", builtin_show);
    lenv_add_builtin(e, "exit", builtin_exit);

    // Math functions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "^", builtin_pow);
    lenv_add_builtin(e, "%", builtin_dif);
}