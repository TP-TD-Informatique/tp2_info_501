#ifndef _SAT_H

#define _SAT_H

// #define NDEBUG       // uncomment to disable debug features (asserts and LOG messages)

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////////////////////
/////////////////////
// DEFINEs and macros
/////////////////////

// number of characters in variable names (including final '\0')
#define NAME_SIZE 32

// macros for dealing with literals:
// literals are strictly positive natural numbers:
//    - 2*l+1 for the positive literal +l
//    - 2*l   for the negative literal -l
// TODO: do we gain anything with that instead of using positive / negative numbers???
#define VARIABLE(l) ((l) >> 1) // get the variable number from a literal
#define SIGN(l) ((l)&1)        // get the polarity (0 for negative, 1 for positive) from a literal
// converting between the internal, strictly positive literal representation (NO) and the signed
// representation (INT)
#define LIT2INT(l) (SIGN(l) ? VARIABLE(l) : -VARIABLE(l))
#define INT2LIT(l) ((l) > 0 ? (2 * (l) + 1) : (-2 * (l)))

// possible states of a variable
#define UNSET -1         // variable is currently unset
#define FALSE 0          // variable is currently set to "false", and "true" hasn't yet been tested
#define TRUE 1           // variable is currently set to "true", and "false" hasn't yet been tested
#define FALSE_WAS_TRUE 2 // variable is currently set to "false", "true" has already been tested
#define TRUE_WAS_FALSE 3 // variable is currently set to "true", "false" has already been tested
#define FORCED_FALSE 4   // variable is currently set to "false" and was forced (unit clause)
#define FORCED_TRUE 5    // variable is currently set to "true" and was forced (unit clause)

#define EOL -1 // end of list, used for watch lists

//////////////////////////////////////////////////
//////////////////////////////////////////////////
// types for representing formula and other things
//////////////////////////////////////////////////

// type for formulas
typedef struct {
    int nb_var;     // number of variables (x_1, x_2, ... x_nb_var)
    int nb_cl;      // number of clauses in the CNF formula
    int nb_lit;     // total number of literals in the CNF formula
    int* Lit;       // array of literals (of size nb_lit)
    int* Cl;        // array of size nb_cl+1 giving the start of each clause in the Lit array
    char** VarName; // array giving the name (if relevant) of each variable
} formula_t;

// type for (partial) assignements
typedef struct {
    int n;       // number of variable in the current assignement
    int* Var;    // array of variables in the current assignement: the array is of size nb_var, but
                 // only the first n entries are relevant. The other ones should be UNSET (-1)...
    char* State; // array of states of the variable. Variables not in the Var array
                 // should have value UNSET (-1)...
} sol_t;

// type for watch lists
typedef struct {
    int* Next;      // array of size nb_cl: for each clause, gives another clause with same
                    // watching literal
    int (*Head)[2]; // 0 -> array of size nb_var: head of watch list for negative literals
                    // 1 -> array of size nb_var: head of watch list for positive literals
} watchlist_t;

// type for active list
typedef struct {
    int last_active; // last element of the active list (or EOL)
    int* NextA;      // array of size nb_var: next variable in the active list (or EOL)
} activelist_t;

//////////////////////////
// boring global variables
extern int VERBOSE;
extern int BUF_SIZE;

///////////////////////////////
// prototypes for the functions

// utils.c file
void LOG(int v, char* format, ...);
/* int trim_blanks(char** buf); */
/* int parse_name_from_comment(char* line, char* name); */

formula_t* parse_formula(FILE* f_in);
void free_formula(formula_t* F);

watchlist_t* init_watchlists(formula_t* F);
void free_watchlist(watchlist_t* W);

activelist_t* init_activelist(formula_t* F, watchlist_t* W);
void free_activelist(activelist_t* A);
int is_active(activelist_t* A, int var);

sol_t* new_sol(int n);
void free_sol(sol_t* S);

void simplify_CNF(formula_t* F, sol_t* S);

// file print.c
void print_struct_formula(formula_t* F);
void print_CNF(formula_t* F);
void pprint_CNF(formula_t* F);
void pprint_watchlists(formula_t* F, watchlist_t* W);
void pprint_activelist(activelist_t* A);
void pprint_sol(sol_t* S);
void pprint_context(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A);
void print_final_solution(formula_t* F, sol_t* S);
void print_negated_solution(formula_t* F, sol_t* S);

// file naive.c
int solve_naive(formula_t* F, sol_t* S);
int backtrack_naive(sol_t* S);
int is_non_false(formula_t* F, sol_t* S);

// file solve.c
int is_solution(formula_t* F, sol_t* S);
int solve(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A, int BCP);
int backtrack(sol_t* S, watchlist_t* W, activelist_t* A);
int update_watch_lists(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A, int lit);
int new_watching_literal(formula_t* F, sol_t* S, int cl);

int next_unit_clause(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A);
int is_unit(formula_t* F, sol_t* S, int cl);
int is_empty_active(activelist_t* A);
void push_active(int var, activelist_t* A);
int pop_active(activelist_t* A);
int preprocess(formula_t* F, sol_t* S);

void LOG_sol(sol_t* S);

// sanity checking
int check_sol(formula_t* F, sol_t* S);
int check_watchlists(formula_t* F, sol_t* S, watchlist_t* W);
int check_activelist(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A);
int check_sanity(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A);

// file test.c
int test(char* cmd, int argc, char** argv);

#endif // _SAT_H

// vim600: set foldmethod=syntax textwidth=100:
