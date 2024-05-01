#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include "lval.h"
#include "count.h"
#include "evaluate.h"

// If it run on windows compile these funcions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char *readline(char *prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);

    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);

    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

// Fake add_history function
void add_history(char *unused) {}

#else
#include <editline/readline.h>
#endif

// Below header may not required so commenting
// it should be better than removing it.
// #include <editline/history.h>

int main(int argc, char **argv)
{
    // Create some Parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    char *language =
        "                                                                                           \
            number      : /-?[0-9]+/ ;                                                              \
            operator    : '+' | '-' | '*' | '/' | '^' | '%' | \"max\" | \"min\" ;                   \
            expr        : <number> | '(' <operator> <expr>+ ')' ;                                   \
            lispy       : /^/ <operator> <expr>+ /$/ ;                                              \
        ";

    // Define them with the following Language
    mpca_lang(MPCA_LANG_DEFAULT,
              language,
              Number, Operator, Expr, Lispy);

    // Print Version and Exit Information
    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+c to Exit!\n");

    // In a never ending loop
    while (1)
    {
        // Display the prompt and get input
        char *input = readline("lispy> ");

        // Add input to history
        add_history(input);

        // Attempt to parse user input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {
            mpc_ast_t *a = r.output;
            lval res = eval(a);
            lval_println(res);
            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // Free retrieved input
        free(input);
    }

    // Undefine and delete Parsers
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    fflush(stdout);
    return 0;
}
