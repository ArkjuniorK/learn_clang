#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include "lval.h"

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
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define them with the following Language
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                           \
                number : /-?[0-9]+/ ;                                     \
                symbol : '+' | '-' | '*' | '/' | '^' | '%'                \
                       | \"list\" | \"head\" | \"tail\" | \"join\"        \
                       | \"eval\" | \"cons\" | \"len\" | \"init\" ;       \
                sexpr  : '(' <expr>* ')' ;                                \
                qexpr  : '{' <expr>* '}' ;                                \
                expr   : <number> | <symbol> | <sexpr> | <qexpr> ;        \
                lispy  : /^/ <expr>* /$/ ;                                \
                ",
              Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    // Print Version and Exit Information
    puts("Lispy Version 0.0.1");
    puts("Press ctrl+c to Exit!\n");

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
            lval *v = lval_eval(lval_read(r.output));
            lval_println(v);
            lval_del(v);
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
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
