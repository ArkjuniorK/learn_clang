#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lval.h"
#include "mpc.h"

/**
 * ------
 * Macros
 * ------
 */

#define LASSERT(args, cond, fmt, ...)                                          \
  if (!(cond)) {                                                               \
    lval *err = lval_err(fmt, ##__VA_ARGS__);                                  \
    lval_del(args);                                                            \
    return err;                                                                \
  }

#define LASSERT_TYPE(func, args, index, expect)                                \
  LASSERT(args, args->cell[index]->type == expect,                             \
          "Function '%s' passed incorrect type for argument %i. "              \
          "Got %s, Expected %s.",                                              \
          func, index, ltype_name(args->cell[index]->type),                    \
          ltype_name(expect))

#define LASSERT_COUNT(func, args, num)                                         \
  LASSERT(args, args->count == num,                                            \
          "Function '%s' passed %i arguments. Expected %i", func, a->count,    \
          num)

#define LASSERT_NOT_EMPTY(func, args, index)                                   \
  LASSERT(args, args->cell[index]->count != 0,                                 \
          "Function '%s' passed {} for argument %i", func, index)

/**
 * -----------------
 * Utility functions
 * -----------------
 */

int lfunc_args(const char *name) {
  if (strcmp(name, "show") == 0 || strcmp(name, "exit") == 0)
    return 1;

  return 0;
}

char *ltype_name(int t) {
  switch (t) {
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
 * Below are some of lval basic functions
 * for various purpuses sush as constructing,
 * reading, evaluating, etc.
 * -----------------------------------------
 */

// Construct number lval type
lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// Construct error lval type
lval *lval_err(char *fmt, ...) {
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
lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_func(char *name, lbuiltin func) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUNC;
  v->builtin = func;
  v->sym = malloc(strlen(name) + 1);
  strcpy(v->sym, name);
  return v;
}

// Construct sexpr lval type
lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->cell = NULL;
  v->count = 0;
  return v;
}

// Construct qexpr lval type
lval *lval_qexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->cell = NULL;
  v->count = 0;
  return v;
}

// Constuct defined function lval type
lval *lval_lambda(lval *formals, lval *body) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUNC;

  v->builtin = NULL;
  v->env = lenv_new();

  v->formals = formals;
  v->body = body;

  return v;
}

// Constuct none lval type
lval *lval_none(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NONE;
  return v;
}

// Increase lval counter and append the given lval it to cell
lval *lval_push(lval *x, lval *y) {
  x->count++;
  x->cell = realloc(x->cell, sizeof(lval *) * x->count);
  x->cell[x->count - 1] = y;
  return x;
}

// Add y to x at the beginning
lval *lval_unshift(lval *x, lval *y) {
  x->count++;
  x->cell = realloc(x->cell, sizeof(lval *) * x->count);
  for (int i = (x->count - 1); i > 0; i--) {
    x->cell[i] = x->cell[i - 1];
  }

  x->cell[0] = y;
  return x;
}

// Pop specific lval from the list. It remove the selected item return it.
lval *lval_pop(lval *v, int i) {
  lval *c = v->cell[i];

  // Shift the memory after the item at "i" over the top
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));

  v->count--;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);

  return c;
}

// Take specific lval from given index and delete the rest
lval *lval_take(lval *v, int i) {
  lval *c = lval_pop(v, i);
  lval_del(v);
  return c;
}

// Copy lval primarily useful for put things in or out from environment
lval *lval_copy(lval *a) {
  lval *v = malloc(sizeof(lval));
  v->type = a->type;

  switch (a->type) {
  // For num and func copy the value directly
  case LVAL_NUM:
    v->num = a->num;
    break;

  case LVAL_FUNC:
    if (a->builtin) {
      v->builtin = a->builtin;
    } else {
      v->builtin = NULL;
      v->env = lenv_copy(a->env);
      v->formals = lval_copy(a->formals);
      v->body = lval_copy(a->body);
    }
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
    for (int i = 0; i < v->count; i++) {
      v->cell[i] = lval_copy(a->cell[i]);
    }
    break;
  }

  return v;
}

// Call the function when s-experssion is evaluates
lval *lval_call(lenv *e, lval *f, lval *a) {
  if (f->builtin) {
    return f->builtin(e, a);
  }

  int given = a->count;
  int total = f->formals->count;

  while (a->count) {
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. "
                      "Got %i. Expected %i",
                      given, total);
    }

    lval *sym = lval_pop(f->formals, 0);
    if (strcmp(sym->sym, "&") == 0) {
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. "
                        "Symbol '&' not followed by single symbol.");
      }

      lval *nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }

    lval *val = lval_pop(a, 0);
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  lval_del(a);

  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. "
                      "Symbol '&' not followed by single symbol.");
    }

    lval_del(lval_pop(f->formals, 0));

    lval *sym = lval_pop(f->formals, 0);
    lval *val = lval_qexpr();

    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  // When all formals are bound, then set
  // environment parent to evaluation and return
  if (f->formals->count == 0) {
    f->env->par = e;
    return builtin_eval(f->env, lval_push(lval_sexpr(), lval_copy(f->body)));
  }

  return lval_copy(f);
}

// Read number type content and construct
lval *lval_read_num(mpc_ast_t *t) {
  errno = 0;
  long n = strtol(t->contents, NULL, 0);
  return errno != ERANGE ? lval_num(n) : lval_err("invalid number");
}

// Read the tree
lval *lval_read(mpc_ast_t *t) {
  if (strstr(t->tag, "number"))
    return lval_read_num(t);

  if (strstr(t->tag, "symbol"))
    return lval_sym(t->contents);

  lval *v = NULL;

  if ((strcmp(t->tag, ">")) == 0 || strstr(t->tag, "sexpr"))
    v = lval_sexpr();

  if (strstr(t->tag, "qexpr"))
    v = lval_qexpr();

  for (int i = 0; i < t->children_num; i++) {
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
lval *lval_eval_sexpr(lenv *e, lval *v) {

  for (int i = 0; i < v->count; i++)
    v->cell[i] = lval_eval(e, v->cell[i]);

  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->count == 0)
    return v;

  if (v->count == 1) {
    // return lval_take(v, 0);
    // If the symbol not allow arguments return it instead
    if (lfunc_args(v->cell[0]->sym) == 0) {
      return lval_take(v, 0);
    }

    lval *f = lval_pop(v, 0);
    lval *r = lval_call(e, f, v);
    lval_del(f);
    return r;
  }

  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_FUNC) {
    lval *err = lval_err("S-Expression start with incorrect type. "
                         "Got %s. Expected %s",
                         ltype_name(f->type), ltype_name(LVAL_FUNC));
    lval_del(f);
    lval_del(v);
    return err;
  }

  lval *r = lval_call(e, f, v);
  lval_del(f);
  return r;
}

lval *lval_eval(lenv *e, lval *v) {
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
  }

  return v;
}

// Join each elements from y to x
lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_push(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

// Delete lval from memory
void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    break;

  case LVAL_FUNC:
    if (!v->builtin) {
      lenv_del(v->env);
      lval_del(v->formals);
      lval_del(v->body);
    }
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
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }

    free(v->cell);
    break;
  }

  free(v);
}

int lval_eq(lval *x, lval *y) {
  if (x->type != y->type) {
    return 0;
  }

  switch (x->type) {
  // Comparation for number
  case LVAL_NUM:
    return (x->num == y->num);

  // Comparation for string values
  case LVAL_ERR:
    return (strcmp(x->err, y->err));
  case LVAL_SYM:
    return (strcmp(x->sym, y->sym));

  // Comparation for builtin function
  // would compare formals and body
  case LVAL_FUNC:
    if (x->builtin || y->builtin) {
      return x->builtin == y->builtin;
    }
    return lval_eq(x->formals, x->formals) && lval_eq(x->body, y->body);

  // Comparation for list would
  // compare individual element
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    if (x->count != y->count) 
      return 0;

    for (int i = 0; i < x->count; i++) {
      if (!lval_eq(x->cell[i], y->cell[i])) 
        return 0;
    }

    return 1;
    break;
  }

  return 0;
}

// Print the exp type of lval
void lval_expr_print(lval *v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    // Do not print trailing space at last element
    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

// Print the `lval`
void lval_print(lval *v) {
  switch (v->type) {
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
    printf("%s", v->sym);
    break;

  case LVAL_FUNC:
    if (v->builtin) {
      printf("<builtin>");
    } else {
      printf("(\\");
      lval_print(v->formals);
      putchar(' ');
      lval_print(v->body);
      printf(")");
    }
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
void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}

/**
 * ---------------------------------------
 * Belows are some builtin based functions
 * ---------------------------------------
 */

// Take the first element from qexpr and remove the rest
lval *builtin_head(lenv *e, lval *a) {
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_COUNT("head", a, 1);
  LASSERT_NOT_EMPTY("head", a, 0);

  lval *f = lval_take(a, 0);
  while (f->count > 1) {
    lval_del(lval_pop(f, 1));
  }

  return f;
}

// Take and remove the first element from qexpr
lval *builtin_tail(lenv *e, lval *a) {
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_COUNT("tail", a, 1);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval *f = lval_take(a, 0);
  lval_del(lval_pop(f, 0));
  return f;
}

lval *builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval *builtin_eval(lenv *e, lval *a) {
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
  LASSERT_COUNT("eval", a, 1);
  LASSERT_NOT_EMPTY("eval", a, 0);

  lval *f = lval_take(a, 0);
  f->type = LVAL_SEXPR;
  return lval_eval(e, f);
}

// Join two or more q-expr
lval *builtin_join(lenv *e, lval *a) {
  LASSERT_COUNT("join", a, 2);

  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("eval", a, i);
  }

  lval *f = lval_pop(a, 0);

  while (a->count) {
    f = lval_join(f, lval_pop(a, 0));
  }

  lval_del(a);
  return f;
}

// Append the given argument to q-expr and
// placed it at the first element
lval *builtin_cons(lenv *e, lval *a) {
  LASSERT_COUNT("cons", a, 2);
  LASSERT_TYPE("cons", a, 0, LVAL_NUM);
  LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

  lval *f = lval_pop(a, 0);
  f = lval_unshift(lval_pop(a, 0), f);

  lval_del(a);
  return f;
}

lval *builtin_len(lenv *e, lval *a) {
  LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
  LASSERT_COUNT("len", a, 1);

  return lval_num(a->cell[0]->count);
}

// Display the first item of q-expr
lval *builtin_init(lenv *e, lval *a) {
  LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
  LASSERT_COUNT("init", a, 1);
  LASSERT_NOT_EMPTY("init", a, 0);

  lval *f = lval_take(a, 0);
  lval_del(lval_pop(f, f->count - 1));

  return f;
}

lval *builtin_var(lenv *e, lval *a, char *func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

  lval *syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot define non-symbol. "
            "Got %s. Expected %s",
            func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
          "Function '%s' passed too many arguments for symbols. "
          "Got %i. Expected %i",
          func, syms->count, a->count - 1);

  for (int i = 0; i < syms->count; i++) {
    // Define it locally
    if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    // Define it globally
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i + 1]);
    }
  }

  lval_del(a);
  return lval_sexpr();
}

lval *builtin_put(lenv *e, lval *a) { return builtin_var(e, a, "="); }

lval *builtin_def(lenv *e, lval *a) { return builtin_var(e, a, "def"); }

lval *builtin_show(lenv *e, lval *a) {
  for (int i = 0; i < e->count; i++) {
    if (e->vals[i]->type == LVAL_NUM)
      printf("%s\n", e->syms[i]);
  }

  lval_del(a);
  return lval_none();
}

lval *builtin_exit(lenv *e, lval *a) {
  LASSERT(a, a->count < 1,
          "Function 'exit' passed too many arguments. "
          "Got %i, Expected %i",
          a->count, 0);

  lenv_del(e);
  lval_del(a);

  exit(EXIT_SUCCESS);
}

lval *builtin_op(lenv *e, lval *a, char *op) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE(op, a, i, LVAL_NUM);
  }

  lval *f = lval_pop(a, 0);
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    f->num = -f->num;
  }

  while (a->count > 0) {
    lval *n = lval_pop(a, 0);

    if (strcmp(op, "+") == 0)
      f->num += n->num;

    if (strcmp(op, "-") == 0)
      f->num -= n->num;

    if (strcmp(op, "*") == 0)
      f->num *= n->num;

    if (strcmp(op, "/") == 0) {
      if (n->num == 0) {
        lval_del(f);
        lval_del(n);
        f = lval_err("Division by zero!");
        break;
      }

      f->num /= n->num;
    }

    if (strcmp(op, "^") == 0)
      f->num = pow(f->num, n->num);

    if (strcmp(op, "%") == 0) {
      if (n->num == 0) {
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

lval *builtin_add(lenv *e, lval *a) { return builtin_op(e, a, "+"); }

lval *builtin_sub(lenv *e, lval *a) { return builtin_op(e, a, "-"); }

lval *builtin_mul(lenv *e, lval *a) { return builtin_op(e, a, "*"); }

lval *builtin_div(lenv *e, lval *a) { return builtin_op(e, a, "/"); }

lval *builtin_pow(lenv *e, lval *a) { return builtin_op(e, a, "^"); }

lval *builtin_dif(lenv *e, lval *a) { return builtin_op(e, a, "%"); }

lval *builtin_ord(lenv *e, lval *a, char *op) {
  LASSERT_COUNT(op, a, 2);
  LASSERT_TYPE(op, a, 0, LVAL_NUM);
  LASSERT_TYPE(op, a, 1, LVAL_NUM);

  int r;

  if (strcmp(op, ">") == 0) {
    r = (a->cell[0]->num > a->cell[1]->num);
  }

  if (strcmp(op, ">=") == 0) {
    r = (a->cell[0]->num >= a->cell[1]->num);
  }

  if (strcmp(op, "<") == 0) {
    r = (a->cell[0]->num < a->cell[1]->num);
  }

  if (strcmp(op, "<=") == 0) {
    r = (a->cell[0]->num <= a->cell[1]->num);
  }

  lval_del(a);
  return lval_num(r);
}

lval *builtin_gt(lenv *e, lval *a) { return builtin_ord(e, a, ">"); }

lval *builtin_ge(lenv *e, lval *a) { return builtin_ord(e, a, ">="); }

lval *builtin_lt(lenv *e, lval *a) { return builtin_ord(e, a, "<"); }

lval *builtin_le(lenv *e, lval *a) { return builtin_ord(e, a, "<="); }

lval *builtin_cmp(lenv *e, lval *a, char *op) {
  LASSERT_COUNT(op, a, 2);

  int r;

  if (strcmp(op, "==") == 0) {
    r = lval_eq(a->cell[0], a->cell[1]);
  }

  if (strcmp(op, "!=") == 0) {
    r = !lval_eq(a->cell[0], a->cell[1]);
  }

  lval_del(a);
  return lval_num(r);
}

lval *builtin_eq(lenv *e, lval *a) { return builtin_cmp(e, a, "=="); }

lval *builtin_ne(lenv *e, lval *a) { return builtin_cmp(e, a, "!="); }

lval *builtin_lgc(lenv *e, lval *a) {

}

lval *builtin_if(lenv *e, lval *a) {
  LASSERT_COUNT("if", a, 3);
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

  // Change both QEXPR expression
  // to SEXPR so it evaluable
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  lval *x;

  // If condition is set as true
  // then evaluate first expression
  // otherwise evaluate the second
  if (a->cell[0]->num) {
    x = lval_eval(e, lval_pop(a, 1));
  } else {
    x = lval_eval(e, lval_pop(a, 2));
  }

  lval_del(a);
  return x;
}

lval *builtin_lambda(lenv *e, lval *a) {
  LASSERT_COUNT("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, a->cell[0]->cell[i]->type == LVAL_SYM,
            "Cannot define non-symbol. Got %s, Expected %s",
            ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }

  lval *formals = lval_pop(a, 0);
  lval *body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval *builtin_func(lenv *e, lval *a) {
  LASSERT_COUNT("func", a, 2);
  LASSERT_TYPE("func", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("func", a, 1, LVAL_QEXPR);

  lval *func_args = lval_pop(a, 0);
  lval *func_body = lval_pop(a, 0);

  lval *ff_args = lval_pop(func_args, 0);
  lval *ff_body = lval_copy(func_args);

  lenv_def(e, ff_args, lval_lambda(ff_body, func_body));

  lval_del(func_args);
  lval_del(func_body);
  lval_del(ff_args);
  lval_del(ff_body);
  lval_del(a);

  return lval_sexpr();
}

/**
 * -------------------------------------
 * Below is some of lenv basic functions
 * -------------------------------------
 */

// Construct empty lenv
lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  e->par = NULL;
  return e;
}

lenv *lenv_copy(lenv *e) {
  lenv *n = malloc(sizeof(lenv));
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc(sizeof(char *) * n->count);
  n->vals = malloc(sizeof(lval *) * n->count);

  for (int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) * 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }

  return n;
}

// Get a copy of value of lenv
lval *lenv_get(lenv *e, lval *k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0)
      return lval_copy(e->vals[i]);
  }

  if (e->par)
    return lenv_get(e->par, k);

  return lval_err("Unbound symbol '%s'", k->sym);
}

// Set or update a value of lenv
// Define variable at innermost of environment
void lenv_put(lenv *e, lval *k, lval *v) {
  // See if a key already exists in environment
  // If it is exists, then replace the current value
  // of the key with the one that user provide
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // Otherwise allocate space for new entries
  e->count++;
  e->syms = realloc(e->syms, sizeof(char *) * e->count);
  e->vals = realloc(e->vals, sizeof(lval *) * e->count);

  // Copy the contents to lenv
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
  e->vals[e->count - 1] = lval_copy(v);
}

// Define variable at outermost of environment
void lenv_def(lenv *e, lval *k, lval *v) {
  while (e->par) {
    e = e->par;
  }

  lenv_put(e, k, v);
}

// Delete lenv and its children
void lenv_del(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }

  free(e->syms);
  free(e->vals);
  free(e);
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_func(name, func);
  lenv_put(e, k, v);
  free(k);
  free(v);
}

// Register all builtin functions
void lenv_add_builtins(lenv *e) {
  // Declarative functions
  lenv_add_builtin(e, "=", builtin_put);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "\\", builtin_lambda);

  // Builtin functions
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "func", builtin_func);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "init", builtin_init);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "show", builtin_show);
  lenv_add_builtin(e, "exit", builtin_exit);

  // Ordering functions
  // lenv_add_builtin(e, "asc", NULL);
  // lenv_add_builtin(e, "desc", NULL);

  // Comparison functions
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "<", builtin_lt);
  lenv_add_builtin(e, ">", builtin_gt);
  lenv_add_builtin(e, "<=", builtin_le);
  lenv_add_builtin(e, ">=", builtin_ge);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_ne);

  // Math functions
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "^", builtin_pow);
  lenv_add_builtin(e, "%", builtin_dif);
}
