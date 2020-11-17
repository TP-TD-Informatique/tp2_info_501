#include "sat.h"

// print the formula in CNF
void print_CNF(formula_t *F) {
    for (int i = 0; i < F->nb_cl; ++i) {
        for (int j = F->Cl[i]; j < F->Cl[i + 1]; ++j) {
            printf("%d ", LIT2INT(F->Lit[j]));
        }
        printf("\n");
    }
}

// check if a solution is indeed a solution
int is_solution(formula_t *F, sol_t *S) {
    for (int i = 0; i < F->nb_cl; ++i) {
        int t = 0;
        for (int j = F->Cl[i]; j < F->Cl[i + 1]; ++j) {
            int state = S->State[VARIABLE(F->Lit[j])];
            int sign = SIGN(F->Lit[j]);
            if (sign && (state == TRUE || state == TRUE_WAS_FALSE)) {
                t = 1;
            } else if (!sign && (state == FALSE || state == FALSE_WAS_TRUE)) {
                t = 1;
            }
        }
        if (t == 0) {
            return 0;
        }
    }
    return 1;
}

int check_sol(formula_t *F, sol_t *S) {
    (void) F;  // pour supprimer le warning "unused variable"
    (void) S;  // pour supprimer le warning "unused variable"
    char SET[1 + F->nb_var];
    (void) SET;
    for (int i = 0; i <= F->nb_var; i++)
        SET[i] = 0;

    // check that S->Var contains UNSET at the end
    // TODO...

    // the variable at S->n may be SET, but its value must be FALSE or TRUE

    // check that variables that appear in the array SET have a state that is different from UNSET
    // TODO...

    return 1;
}

// main function: look for a solution to satisfy the global formula
int solve(formula_t *F, sol_t *S, watchlist_t *W, activelist_t *A, int BCP) {

    int cpt = 0;

    int current_var; // current variable
    int current_lit; // current literal: either 2*current_var+1 (for negative literal) or 2*n (for positive literal)

    // we don't stop until we found values for all the variables (or we've tried everything)
    while (0 <= S->n && S->n < F->nb_var) {
        cpt++;
        LOG(2, "\n>>>>>> passage %d dans la boucle, n = %d...\n", cpt, S->n);

        assert(check_sanity(F, S, W, A));

        // we need to choose a value for the n-th variable in Sol
        if (S->Var[S->n] == UNSET) { // if this variable is unset

            /***** CHOOSE A NEW VARIABLE *****/
            if (A == NULL) { // if there is no active list, we simply take the next variable
                current_var = S->n + 1;
                // and it's value will be TRUE (1) or FALSE (0) depending on whether X_n or ~X_n is watched
                S->State[current_var] = W->Head[current_var][0] == EOL || W->Head[current_var][1] != EOL;

            } else if (BCP == 0) { // if there is an active list but we don't do constraint propagation,
                // we take the first active variable

                if (is_empty_active(A)) {
                    // if there are no active variable, we've actually finished! The formula is satisfiable...
                    break;
                }
                // otherwise, we take the first active variable
                current_var = A->NextA[A->last_active];
                // and it's value will be TRUE (1) or FALSE (0) depending on whether X_n or ~X_n is watched
                S->State[current_var] = W->Head[current_var][0] == EOL || W->Head[current_var][1] != EOL;

            } else { // if there is an active list and we do constraint propagation (DPLL), we look for a forced literal
                // (a unit clause)

                if (is_empty_active(A)) {
                    // if there are no active variable, we've actually finished! The formula is satisfiable...
                    break;
                }

                // otherwise, we look for a forced literal
                int cl = next_unit_clause(F, S, W, A);

                if (cl < 0) {
                    // if there is no forced literal, we take the first active variable
                    current_var = A->NextA[A->last_active];
                    // and it's value will be TRUE (1) or FALSE (0) depending on whether X_n or ~X_n is watched
                    S->State[current_var] = W->Head[current_var][0] == EOL || W->Head[current_var][1] != EOL;
                } else {
                    // there was a unit clause, we use its leading (unique) literal!
                    current_lit = F->Lit[F->Cl[cl]];
                    LOG(3, "La clause %d est unitaire : elle ne contient que %d\n", cl, LIT2INT(current_lit));
                    current_var = VARIABLE(current_lit);
                    S->State[current_var] = 4 + SIGN(current_lit);
                }
            }
            /***** END OF CHOOSE A NEW VARIABLE *****/

            // we now have a candidate for the n-th variable of the partial solution
            S->Var[S->n] = current_var;

        } else { // if the n-th variable in the partial solution was already set, we need to change its value
            current_var = S->Var[S->n];
            S->State[current_var] = 3 - S->State[current_var];
        }

        current_lit = 2 * current_var + (S->State[current_var] & 1);

        if (A != NULL) { // we may need to update the active list
            // If var was set as a first choice (state 0, 1), we need to remove it from the active list
            // (in states 2, 3, the variable had already been removed)
            // TODO : gérer la suppression de la liste des variables actives
        }

        if (VERBOSE == 3) {
            LOG(3, "> > >  solution courante : ");
            pprint_sol(S);
        } else if (VERBOSE > 3) {
            pprint_context(F, S, W, A);
        }

        // we now need to update the appropriate watch lists of current_var:
        // if it was set to TRUE, we need to update the FALSE watch_list, and vice-versa
        if (update_watch_lists(F, S, W, A, current_lit ^ 1) > 0) {
            // continue with next variable
            S->n++;
        } else { // otherwise, we need to backtrack to the last previously set
            // variable that has only been tested on one boolean value
            S->n = backtrack(S, W, A);
            LOG(2, "< < <  backtrack: retour à n = %d\n", S->n);
        }
    }
    assert(check_sanity(F, S, W, A));

    LOG(2, "%d solutions essayées\n", cpt);
    if (S->n < 0) {
        return 0;
    } else {
        return 1;
    }
}

// given a partial solution, backtrack to the last position where a choice was made.
// the return value is the new index for the last variable in Sol, but this value is also updated inside S->
int backtrack(sol_t *S, watchlist_t *W, activelist_t *A) {
    (void) W; // to remove unused argument warning

    // states 0 and 1 correspond to variables that have been tested on a single value. We can stop
    // backtracking as soon as we find such a value: we'll test the other value.
    while (S->n >= 0 && S->State[S->Var[S->n]] >= 2) {

        int x = S->Var[S->n];

        // states 2 and 3 correspond to variables that have been
        // tested on 2 values, and states 4 and 5 to forced values.
        // we need to look further: we remove the variables from the branch
        S->State[x] = UNSET;  // on rénitialise cette variable
        S->Var[S->n] = UNSET; // on la supprime de la solution courante
        S->n--;

        if (A != NULL) {
            // we just removed a variable from the current solution, we may need to put it into the active list
            // TODO : gérer l'insertion dans la liste des variables actives
        }
    }
    // NOTE : si on renvoie -1, c'est qu'il n'était pas possible de revenir en arrière !
    // La formule n'était donc pas satisfiable.
    return S->n;
}

// update watch lists for lit (a literal that has just become false)
// make lit non-watched in all the clauses that where watched by it
// if no other literal can watch the clause, it means the clause
// becomes false and we'll need to backtrack
// NOTE: returns 0 in case an empty clause is found
int update_watch_lists(formula_t *F, sol_t *S, watchlist_t *W, activelist_t *A, int lit) {
    int var = VARIABLE(lit);

    int next_cl;
    for (int cl = W->Head[var][SIGN(lit)]; cl != EOL; cl = next_cl) {
        // BEWARE, W->Next[cl] may change during update, we need to save the value in a variable "next_cl"...
        next_cl = W->Next[cl];

        // get index of a new watching literal in the clause
        int idx = new_watching_literal(F, S, cl);

        if (idx == -1) {
            LOG(3, "! ! !  La clause %d (", cl);
            for (int k = F->Cl[cl]; k < F->Cl[cl + 1]; k++) {
                if (k > F->Cl[cl]) {
                    LOG(3, " ");
                }
                LOG(3, "%d", LIT2INT(F->Lit[k]));
            }
            LOG(3, ") est fausse car tous ces littéraux sont faux.\n");
            // we might have modified some clauses before that, and the watch list for the literal now needs to
            // start at this clause.
            W->Head[var][SIGN(lit)] = cl;
            // NOTE: sanity isn't guaranteed here!
            return 0;
        }

        // otherwise, we have a new watching literal (or the clause is true, which doesn't hurt...)
        // we swap the old and new watching literals
        int new_lit = F->Lit[idx];
        int new_var = VARIABLE(new_lit);
        F->Lit[F->Cl[cl]] = new_lit;
        F->Lit[idx] = lit;

        if (A != NULL) {
            // add the variable for the new watching literal in the active list (if it is UNSET and not already active)
            // TODO : gérer l'insertion dans la liste des variables actives
        }

        // and update the watch list
        W->Next[cl] = W->Head[new_var][SIGN(new_lit)];
        W->Head[new_var][SIGN(new_lit)] = cl;
    }

    // lit isn't watching any clause anymore
    W->Head[var][SIGN(lit)] = EOL;

    return 1;
}

// look for a new literal to serve as the watcher for clause ``cl``
// returns the index (in Lit array) of this literal on success
// returns -1 if lit was the last non-false literal in the clause
int new_watching_literal(formula_t *F, sol_t *S, int cl) {
    (void) F; // to remove unused argument warning
    (void) S; // to remove unused argument warning
    (void) cl; // to remove unused argument warning
    return -1;
}

// is the given clause a unit clause?
// NOTE: we assume the leading literal is unset
int is_unit(formula_t *F, sol_t *S, int cl) {
    assert(S->State[VARIABLE(F->Lit[F->Cl[cl]])] == UNSET);
    (void) F; // to remove unused argument warning
    (void) S; // to remove unused argument warning
    (void) cl; // to remove unused argument warning
    return 1;
}

// vim600: set foldmethod=syntax textwidth=120:
