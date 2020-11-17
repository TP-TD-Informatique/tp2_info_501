#include "sat.h"

int solve_naive(formula_t* F, sol_t* S)
{
    int cpt = 0;
    while (0 <= S->n && S->n < F->nb_var) {
        assert(check_sol(F, S));
        cpt++;
        LOG(2, "\n>>>>>> passage %d dans la boucle, n = %d...\n", cpt, S->n);

        // on regarde la n-ème variable de la solution actuelle :
        if (S->Var[S->n] == UNSET) {
            // il faut choisir une variable à ajouter à la solution actuelle
            // on peut prendre la variable ``n``.
            // (Attention au décalage de 1 : les variables commencent à 1.)
            S->Var[S->n] = S->n + 1;
            // il faut aussi choisir une valeur pour cette variable
            // on peut prendre VRAI.
            S->State[S->Var[S->n]] = TRUE;
        } else if (S->State[S->Var[S->n]] == TRUE || S->State[S->Var[S->n]] == FALSE) {
            // si la variable avait déjà une valeur, il faut en choisir une autre
            // on change la valeur de TRUE (1) à FALSE_WAS_TRUE (2) et de FALSE (0) à TRUE_WAS_FALSE
            // (3)
            S->State[S->Var[S->n]] = 3 - S->State[S->Var[S->n]];
        } else {
            fprintf(stderr, "BUG: CECI NE DEVRAIT JAMAIS ARRIVER !\n");
            exit(7);
        }
        if (VERBOSE == 3) {
            LOG(3, "> > >  solution courante : ");
            pprint_sol(S);
        } else if (VERBOSE > 3) {
            pprint_context(F, S, NULL, NULL);
        }

        if (is_non_false(F, S)) {
            // si les valeurs des variables ne rendent pas la formule fausse, on continue
            S->n++;
        } else {
            // sinon, il faut revenir en arrière !
            backtrack_naive(S);
            LOG(2, "< < <  backtrack: retour à n = %d\n", S->n);
        }
    }
    LOG(2, "%d solutions essayées\n", cpt);
    if (S->n < 0) {
        // non satisfiable
        return 0;
    } else {
        // satisfiable
        assert(S->n == F->nb_var);
        return 1;
    }
}

int backtrack_naive(sol_t* S)
{
    // tant que la dernière variable de la solution à été testée sur les 2 valeurs
    while (S->n >= 0
        && (S->State[S->Var[S->n]] == TRUE_WAS_FALSE || S->State[S->Var[S->n]] == FALSE_WAS_TRUE)) {
        S->State[S->Var[S->n]] = UNSET; // on rénitialise cette variable
        S->Var[S->n] = UNSET;           // on la supprime de la solution courante
        S->n--;
    }
    // NOTE : si on renvoie -1, c'est qu'il n'était pas possible de revenir en arrière !
    // La formule n'était donc pas satisfiable.
    return S->n;
}

int is_non_false(formula_t* F, sol_t* S)
{
    // pour toutes les clauses
    for (int cl = 0; cl < F->nb_cl; cl++) {
        // on calcule la valeur de vérité
        int clause_value = 0;
        // pour tous les littéraux de la clause
        for (int k = F->Cl[cl]; k < F->Cl[cl + 1]; k++) {
            int lit = F->Lit[k];     // littéral courant
            int x = VARIABLE(lit);   // variable correspondante
            int s = SIGN(lit);       // signe correspondant
            int v = S->State[x] & 1; // valeur de la variable dans la solution
            // si ce littéral n'est pas faux, la clause n'est pas fausse !
            if (S->State[x] == UNSET || s == v) {
                clause_value = 1;
                break;
            }
        }
        if (clause_value == 0) {
            // si la clause est fausse (car elle ne contient que des littéraux FAUX)
            // la formule est fausse
            LOG(3, "! ! !  La clause %d (", cl);
            for (int k = F->Cl[cl]; k < F->Cl[cl + 1]; k++) {
                if (k > F->Cl[cl]) {
                    LOG(3, " ");
                }
                LOG(3, "%d", LIT2INT(F->Lit[k]));
            }
            LOG(3, ") est fausse car tous ces littéraux sont faux.\n");
            return 0;
        }
    }
    return 1;
}

// vim600: set foldmethod=syntax textwidth=100:
