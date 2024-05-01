#ifndef count_h
#define count_h

#include "mpc.h"

int count_leaves(mpc_ast_t *t);
int count_branches(mpc_ast_t *t);
int count_max_child(mpc_ast_t *t);

#endif