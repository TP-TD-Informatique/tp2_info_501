#include "sat.h"

// print the content of the arrays of a formula
void print_struct_formula(formula_t* F)
{
    fprintf(stderr, "nb_var = %d\n", F->nb_var);
    fprintf(stderr, "nb_cl  = %d\n", F->nb_cl);
    fprintf(stderr, "nb_lit = %d\n", F->nb_lit);
    fprintf(stderr, "\n");
    for (int i = 0; i <= F->nb_cl; i++) {
        fprintf(stderr, "Cl[%d] = (%d)\n", i, F->Cl[i]);
    }
    fprintf(stderr, "\n");
    for (int i = 0; i < F->nb_lit; i++) {
        fprintf(stderr, "Lit[%d] = %d, càd %d\n", i, F->Lit[i], LIT2INT(F->Lit[i]));
    }
    fprintf(stderr, "\n");
    for (int i = 1; i <= F->nb_var; i++) {
        fprintf(stderr, "VarName[%d] = %s\n", i, F->VarName[i]);
    }
    fprintf(stderr, "\n");
}

int pprint_index(int n)
{
    char* Sub[10] = { "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉" };
    int p = 1;
    while (p <= n)
        p *= 10;
    int s = 0;
    for (p = p / 10; p != 0; p /= 10) {
        fprintf(stderr, "%s", Sub[(n / p) % 10]);
        s++;
    }
    return s;
}

int pprint_var(int var)
{
    int s = 0;
    fprintf(stderr, "X");
    s++;
    s += pprint_index(var);
    return s;
}

int pprint_lit(int lit)
{
    int s = 0;
    if (SIGN(lit) == 0) {
        fprintf(stderr, "¬");
        s++;
    }
    s += pprint_var(VARIABLE(lit));
    return s;
}

void pprint_CNF(formula_t* F)
{
    char sep = F->nb_lit > 50 ? '\n' : ' ';
    for (int i = 0; i < F->nb_cl; i++) {
        if (i > 0)
            fprintf(stderr, " ∧%c", sep);
        fprintf(stderr, "(");
        for (int j = F->Cl[i]; j < F->Cl[i + 1]; j++) {
            if (j > F->Cl[i])
                fprintf(stderr, " ∨ ");
            pprint_lit(F->Lit[j]);
        }
        fprintf(stderr, ")");
    }
    fprintf(stderr, "\n");
}

// print all the active lists
void pprint_watchlists(formula_t* F, watchlist_t* W)
{
    for (int x = 1; x <= F->nb_var; x++) {
        if (W->Head[x][0] == EOL && W->Head[x][1] == EOL) {
            continue;
        }
        if (W->Head[x][0] != EOL) {
            fprintf(stderr, "       ¬");
            pprint_var(x);
            fprintf(stderr, ": ");
            for (int cl = W->Head[x][0]; cl != EOL; cl = W->Next[cl]) {
                fprintf(stderr, "(%d) ", cl);
            }
            fprintf(stderr, "\n");
        }
        if (W->Head[x][1] != EOL) {
            fprintf(stderr, "        ");
            pprint_var(x);
            fprintf(stderr, ": ");
            for (int cl = W->Head[x][1]; cl != EOL; cl = W->Next[cl]) {
                fprintf(stderr, "(%d) ", cl);
            }
            fprintf(stderr, "\n");
        }
    }
}

// print an active list
void pprint_activelist(activelist_t* A)
{
    if (A->last_active == EOL) {
        fprintf(stderr, "⊥\n");
    } else {
        int v, p;
        for (v = A->NextA[A->last_active], p = 0; p != A->last_active; p = v, v = A->NextA[v]) {
            pprint_var(v);
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
    }
}

// print a solution
void pprint_sol(sol_t* S)
{
    for (int i = 0; i <= S->n; i++) {
        int x = S->Var[i];
        if (x != UNSET && S->State[x] != UNSET) {
            if (S->State[x] & 1) {
                fprintf(stderr, " ");
            } else {
                fprintf(stderr, "¬");
            }
            pprint_var(x);
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n");
}

// pretty print everything in the context
void pprint_context(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A)
{
    // print the formula, then the values (tabulated under the literals), then the clause numbers
    (void)F;
    (void)S;
    fprintf(stderr, "> > >  current solution: ");
    pprint_sol(S);
    // print CNF formula, keeping the position of each literal
    fprintf(stderr, "> > >  formula:\n");
    int Pos[F->nb_lit];
    int p = 0;
    for (int cl = 0; cl < F->nb_cl; cl++) {
        if (cl > 0) {
            fprintf(stderr, " ∧ ");
            p += 3;
        }
        fprintf(stderr, "(");
        p++;
        for (int i = F->Cl[cl]; i < F->Cl[cl + 1]; i++) {
            if (i > F->Cl[cl]) {
                fprintf(stderr, " ∨ ");
                p += 3;
            }
            int lit = F->Lit[i];
            Pos[i] = p + 1 - SIGN(lit);
            p += pprint_lit(lit);
        }
        fprintf(stderr, ")");
        p++;
    }
    fprintf(stderr, "\n");

    int i = 0;
    for (int j = 0; j < p; j++) {
        if (j == Pos[i]) {
            int lit = F->Lit[i];
            int x = VARIABLE(lit);
            int s = SIGN(lit);
            if (S->State[x] == UNSET) {
                fprintf(stderr, "?");
            } else if ((S->State[x] & 1) == s) {
                fprintf(stderr, "1");
            } else {
                fprintf(stderr, "0");
            }
            if (S->State[x] == FORCED_FALSE || S->State[x] == FORCED_TRUE) {
                fprintf(stderr, "!");
                j++;
            } else if (S->State[x] == FALSE_WAS_TRUE || S->State[x] == TRUE_WAS_FALSE) {
                fprintf(stderr, "*");
                j++;
            }
            i++;
        } else {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n");

    if (A != NULL) {
        fprintf(stderr, "> > >  active list: ");
        pprint_activelist(A);
    }
    if (W != NULL) {
        fprintf(stderr, "> > >  watch lists:\n");
        pprint_watchlists(F, W);
    }
}

// print a solution
void print_final_solution(formula_t* F, sol_t* S)
{
    for (int i = 0; i < S->n; i++) {
        int x = S->Var[i];
        if (F == NULL || F->VarName[x] == NULL) {
            printf("%d ", (S->State[x] & 1) ? x : -x);
        } else {
            printf("%s%s ", (S->State[x] & 1) ? "" : "~", F->VarName[x]);
        }
    }
    printf("\n");
}

// print the negation of a solution as a clause
// (usefull to add to a file to look for other solutions)
// if F is NULL, print in DIMACS format, else in Knuth's SAT format
void print_negated_solution(formula_t* F, sol_t* S)
{
    if (F == NULL) {
        printf("c negation of solution:\n");
    } else {
        printf("~ negation of solution:\n");
    }
    int missing_names = 0;
    for (int i = 0; i < S->n; i++) {
        int x = S->Var[i];
        if (F == NULL) {
            printf("%d ", (S->State[x] & 1) ? -x : x);
        } else {
            if (F->VarName[x] == NULL) {
                missing_names++;
            } else {
                printf("%s%s ", (S->State[x] & 1) ? "~" : "", F->VarName[x]);
            }
        }
    }
    if (F == NULL) {
        printf("0\n");
    } else {
        if (missing_names == S->n) {
            printf("~ no variable name was present, the solution was ignored\n");
        } else if (missing_names > 0) {
            printf("\n");
            printf("~ some variables were ignored as they had no name\n");
        } else {
            printf("\n");
        }
    }
}

// vim600: set foldmethod=syntax textwidth=120:
