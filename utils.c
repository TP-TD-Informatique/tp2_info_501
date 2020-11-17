#include "sat.h"

/////////////////
// misc functions

// log messages on stderr
void LOG(int v, char* format, ...)
{
    fflush(NULL);
    if (v > VERBOSE)
        return;
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(NULL);
}

// remove leading blanks (' ' and '\t') from ``buf`` and return number of removed characters
int trim_blanks(char** buf)
{
    int n = 0;
    while ((**buf == ' ' || **buf == '\t') && **buf != '\0') {
        (*buf)++;
        n++;
    }
    return n;
}

////////////////////////
// dealing with formulas

// parse a name and variable index from a string:
// when line is of the form  NAME -> IDX, the NAME is recorded into ``name``, and IDX is returned
int parse_name_from_comment(char* line, char* name)
{
    trim_blanks(&line);
    char* buf = line;

    // get possible variable name
    int n = 0;
    while (1) {
        if (*buf == ' ' || *buf == '\t' || *buf == '\0') {
            name[n] = '\0';
            break;
        }
        if (*buf < '!' || *buf > '}') {
            return 0;
        }
        if (*buf >= '!' && *buf <= '}') {
            buf++;
            n++;
        }
    }

    trim_blanks(&buf);

    // get possible "->"
    if (buf[0] != '-' || buf[1] != '>') {
        return 0;
    }
    buf += 2;

    // get possible variable index number
    char* rest;
    int idx = strtol(buf, &rest, 10);
    if (rest == buf) {
        return 0;
    }

    // check rest of the line is empty
    trim_blanks(&buf);
    if (*buf == '\0') {
        return 0;
    }

    if (n > NAME_SIZE) {
        printf("variable is too long: %.*s", NAME_SIZE, name);
        exit(1);
    }
    strncpy(name, line, n + 1);
    name[n] = '\0';
    return idx;
}

// parse a formula from the command line
formula_t* parse_formula(FILE* f_in)
{
    int current_clause = 0; // index of the current clause read from file
    int current_lit = 0;    // index of the current literal read from file
    int nb_var = 0;         // number of variables read from file
    int size_Cl = 1;        // actual size of the Cl array
    int size_Lit = 1;       // actual size of Lit array
    int size_VarName = 1;   // actual size of VarName array

    int* Cl = malloc(size_Cl * sizeof(int));
    int* Lit = malloc(size_Lit * sizeof(int));
    char** VarName = malloc(size_VarName * sizeof(char*));
    VarName[0] = NULL;

    char name[NAME_SIZE];
    int idx;

    char line[BUF_SIZE];

    while (fgets(line, BUF_SIZE, f_in)) { // parse all lines from file
        int n = strlen(line);
        if (line[n - 1] != '\n') { // error, buffer too small
            fprintf(stderr, "line too long\n");
            exit(3);
        }
        line[n - 1] = '\0';

        char* buf = line;
        trim_blanks(&buf);

        if (*buf == 'c') {
            if ((idx = parse_name_from_comment(++buf, name)) > 0) {
                // we have a name and an index. We insert it into the name
                // field of the corresponding var
                if (idx >= size_VarName) {
                    int size = size_VarName;
                    while (size <= idx) {
                        size *= 2;
                    }
                    VarName = realloc(VarName, size * sizeof(char*));
                    for (int i = size_VarName; i < size; i++) {
                        VarName[i] = NULL;
                    }
                    size_VarName = size;
                }
                VarName[idx] = realloc(VarName[idx], NAME_SIZE * sizeof(char));
                strncpy(VarName[idx], name, NAME_SIZE);
                nb_var = idx > nb_var ? idx : nb_var;
            }
            continue;
        }
        if (*buf == '\0') { // ignore empty lines
            continue;
        }
        if (*buf == 'p') { // FIXME: I could parse the header to allocate
                           // directly the appropriate
            continue;      // sizes for VAR and CL
        }

        if (size_Cl <= current_clause) { // realloc CL array if necessary
            size_Cl *= 2;
            Cl = realloc(Cl, size_Cl * sizeof(int));
        }
        assert(size_Cl > current_clause);
        Cl[current_clause] = current_lit;
        while (1) { // parse all literals from current line buffer
            int l = strtol(buf, &buf, 10);
            if (l == 0) {
                current_clause++;
                break;
            }
            // update number of variables
            if (l > 0 && l > nb_var) {
                nb_var = l;
            } else if (l < 0 && -l > nb_var) {
                nb_var = -l;
            }
            if (size_Lit <= current_lit) { // realloc Lit array if necessary
                size_Lit *= 2;
                Lit = realloc(Lit, size_Lit * sizeof(int));
            }
            assert(size_Lit > current_lit);
            Lit[current_lit] = INT2LIT(l);
            current_lit++;
        }
    }
    // realloc LIT array to actual size
    Lit = realloc(Lit, current_lit * sizeof(int));

    // realloc CL array to actual size
    Cl = realloc(Cl, (current_clause + 1) * sizeof(int));

    // make sure the last cell is correct
    Cl[current_clause] = current_lit;

    // realloc VarName array to actual nb of variable
    for (int i = nb_var + 1; i < size_VarName; i++) {
        free(VarName[i]);
    }
    VarName = realloc(VarName, (nb_var + 1) * sizeof(char*));

    formula_t* F = malloc(sizeof(formula_t));
    F->nb_var = nb_var;
    F->nb_cl = current_clause;
    F->nb_lit = current_lit;
    F->Lit = Lit;
    F->Cl = Cl;
    F->VarName = VarName;
    return F;
}

// free a formula
void free_formula(formula_t* F)
{
    if (F == NULL)
        return;
    free(F->Lit);
    free(F->Cl);
    for (int i = 0; i <= F->nb_var; i++) {
        free(F->VarName[i]);
    }
    free(F->VarName);
    free(F);
}

///////////////////////////
// dealing with watch lists

// initializes the watch lists
watchlist_t* init_watchlists(formula_t* F)
{
    watchlist_t* W = malloc(sizeof(watchlist_t));
    W->Next = malloc(F->nb_cl * sizeof(int));
    W->Head = malloc((F->nb_var + 1) * sizeof(int[2]));

    // initialize watch lists heads
    for (int i = 1; i <= F->nb_var; i++) {
        W->Head[i][0] = EOL;
        W->Head[i][1] = EOL;
    }

    // initialize watch lists
    for (int i = 0; i < F->nb_cl; i++) {
        if (F->Cl[i] == F->Cl[i + 1]) { // the clause is empty
            printf("Empty clause in initial problem...\n");
            printf("UNSATISFIABLE\n");
            exit(2);
        }
        int l = F->Lit[F->Cl[i]];
        int x = VARIABLE(l);
        int s = SIGN(l);
        int tmp = W->Head[x][s];
        W->Head[x][s] = i;
        W->Next[i] = tmp;
    }
    return W;
}

// free a watchlist
void free_watchlist(watchlist_t* W)
{
    if (W == NULL)
        return;
    free(W->Head);
    free(W->Next);
    free(W);
}

////////////////////////////
// dealing with active lists

// initializes the active list
activelist_t* init_activelist(formula_t* F, watchlist_t* W)
{
    activelist_t* A = malloc(sizeof(activelist_t));
    A->NextA = malloc((1 + F->nb_var) * sizeof(int));
    /* not necessary
    for (int i = 0; i <= F->nb_var; i++) {
        A->NextA[i] = UNSET;
    }
    */

    // initialize active list
    A->last_active = EOL;
    int prev = 1;
    for (int k = F->nb_var; k > 0; k--) {
        // TODO valgrind error
        if (W->Head[k][0] != EOL || W->Head[k][1] != EOL) {
            if (A->last_active == EOL) {
                A->last_active = k;
            }
            A->NextA[k] = prev;
            prev = k;
        }
    }
    if (A->last_active != EOL) {
        A->NextA[A->last_active] = prev;
    }
    return A;
}

// free an active list
void free_activelist(activelist_t* A)
{
    if (A == NULL)
        return;
    free(A->NextA);
    free(A);
}

// check if a variable is in the active list
// (should only be used for debug purposes)
int is_active(activelist_t* A, int var)
{
    if (A->last_active == EOL) {
        return 0;
    }
    int prev, v;
    for (prev = 0, v = A->NextA[A->last_active]; prev != A->last_active;
         prev = v, v = A->NextA[v]) {
        if (v == var) {
            return 1;
        }
    }
    return 0;
}

// check if the active list is empty
int is_empty_active(activelist_t* A) { return A->last_active == EOL; }

// insert a variable at head of active list
void push_active(int var, activelist_t* A)
{
    assert(!is_active(A, var));
    if (A->last_active == EOL) {
        A->last_active = var;
        A->NextA[A->last_active] = var;
        return;
    }
    int head = A->NextA[A->last_active];
    A->NextA[A->last_active] = var;
    A->NextA[var] = head;
    A->last_active = var;
}

// remove the head from the active list
int pop_active(activelist_t* A)
{
    assert(A->last_active != EOL);
    int head = A->NextA[A->last_active];
    if (A->last_active == head) {
        A->last_active = EOL;
    } else {
        A->NextA[A->last_active] = A->NextA[A->NextA[A->last_active]];
    }
    return head;
}

// return the index of a unit clause, if possible
// otherwise, -1 is returned
int next_unit_clause(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A)
{
    assert(!is_empty_active(A));
    int prev = 0;
    int v = A->NextA[A->last_active];

    while (1) {
        // is there a unit clause watched by v? We look at all the clauses
        // watched by v
        for (int cl = W->Head[v][0]; cl != EOL; cl = W->Next[cl]) {
            if (is_unit(F, S, cl)) {
                // success, we found a unit clause watched by v
                // we update the active list to start searching from here
                A->last_active = prev != 0 ? prev : A->last_active;
                return cl;
            }
        }

        // is there a unit clause watched by ~v
        for (int cl = W->Head[v][1]; cl != EOL; cl = W->Next[cl]) {
            if (is_unit(F, S, cl)) {
                // success, we found a unit clause watched by v
                // we update the active list to start searching from here
                A->last_active = prev != 0 ? prev : A->last_active;
                return cl;
            }
        }

        // next variable
        prev = v;
        v = A->NextA[v];
        if (prev == A->last_active) {
            return -1;
        }
    }
    assert(0);
}

/////////////////////////
// dealing with solutions

// initialize an empty partial solution for a given number of variables
sol_t* new_sol(int n)
{
    sol_t* S = malloc(sizeof(sol_t));
    S->Var = malloc(n * sizeof(int));
    S->State = malloc((1 + n) * sizeof(int));
    // variables are initially unset
    for (int i = 0; i <= n; i++) {
        S->State[i] = UNSET;
    }
    for (int i = 0; i < n; i++) {
        S->Var[i] = UNSET;
    }
    S->n = 0; // nombre de variables dans la solution actuelle (Sol)
    return S;
}

// free a solution
void free_sol(sol_t* S)
{
    if (S == NULL)
        return;
    free(S->State);
    free(S->Var);
    free(S);
}

// simplify a formula given a partial solution (useful for preprocessing)
void simplify_CNF(formula_t* F, sol_t* S)
{
    int current_new_clause = 0;
    int current_new_i = 0;
    for (int cl = 0; cl < F->nb_cl; cl++) {
        F->Cl[current_new_clause] = current_new_i;
        for (int i = F->Cl[cl]; i < F->Cl[cl + 1]; i++) {
            int lit = F->Lit[i];
            int x = VARIABLE(lit);
            int s = SIGN(lit);

            // ignore false literals
            if (S->State[x] != UNSET && (S->State[x] & 1) != s) {
                continue;
            }

            // ignore clauses with true literal
            if (S->State[x] != UNSET && (S->State[x] & 1) == s) {
                current_new_i = F->Cl[current_new_clause];
                current_new_clause--;
                break;
            }

            // keep UNSET literal
            F->Lit[current_new_i] = F->Lit[i];
            current_new_i++;
        }
        current_new_clause++;
    }
    F->nb_cl = current_new_clause;
    F->nb_lit = current_new_i;
    F->Cl[F->nb_cl] = F->nb_lit;
    F->Lit = realloc(F->Lit, F->nb_lit * sizeof(int));
    F->Cl = realloc(F->Cl, (F->nb_cl + 1) * sizeof(int));
}

int preprocess(formula_t* F, sol_t* S)
{
    watchlist_t* W = init_watchlists(F);
    activelist_t* A = init_activelist(F, W);

    while (1) {
        if (A->last_active == EOL) {
            return 1;
        }
        int cl = next_unit_clause(F, S, W, A);
        if (cl < 0) {
            break;
        }
        int lit = F->Lit[F->Cl[cl]];
        int x = VARIABLE(lit);
        int _x = pop_active(A);
        (void)_x;
        assert(x == _x);
        int s = SIGN(lit);
        S->Var[S->n] = x;
        S->n++;
        S->State[x] = 4 + s;
        if (update_watch_lists(F, S, W, A, lit ^ 1) == 0) {
            return -1;
        }
    }
    free_watchlist(W);
    free_activelist(A);
    simplify_CNF(F, S);
    return 0;
}

int check_watchlists(formula_t* F, sol_t* S, watchlist_t* W)
{
    // check that each clause appears in a the watch list of its leading literal
    // and that this literal is non false
    // NOTE: the variable at S->n might be set in case we are backtracking. Its value should be
    // ignored... We "UNSET" it, and put it back at the end.
    int old_value = -123;
    if (0 <= S->n && S->n < F->nb_var && S->Var[S->n] != UNSET) {
        old_value = S->State[S->Var[S->n]];
        S->State[S->Var[S->n]] = UNSET;
    }

    char CHECKED[F->nb_cl];
    (void)CHECKED;
    for (int cl = 0; cl < F->nb_cl; cl++) {
        CHECKED[cl] = 0;
    }
    for (int x = 1; x <= F->nb_var; x++) {
        for (int s = 0; s < 2; s++) {
            for (int cl = W->Head[x][s]; cl != EOL; cl = W->Next[cl]) {
                assert(F->Cl[cl] < F->Cl[cl + 1]);
                int lit = F->Lit[F->Cl[cl]];
                (void)lit;
                assert(lit == 2 * x + s);
                assert((S->State[x] == UNSET) || ((S->State[x] & 1) == s));
                assert(CHECKED[cl] == 0);
                CHECKED[cl] = 1;
            }
        }
    }
    for (int cl = 0; cl < F->nb_cl; cl++) {
        assert(CHECKED[cl] == 1);
    }

    if (0 <= S->n && S->n < F->nb_var && S->Var[S->n] != UNSET) {
        assert(old_value != -123);
        S->State[S->Var[S->n]] = old_value;
    }
    return 1;
}

int check_activelist(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A)
{
    (void)S;
    (void)W;
    if (A == NULL || A->last_active == EOL)
        return 1;

    char SEEN[F->nb_var + 1];
    for (int x = 1; x <= F->nb_var; x++) {
        SEEN[x] = 0;
    }
    int p, v;
    for (p = 0, v = A->NextA[A->last_active]; p != A->last_active; p = v, v = A->NextA[v]) {
        assert(W->Head[v][0] != EOL || W->Head[v][1] != EOL);
        SEEN[v] = 1;
    }
    for (int x = 1; x <= F->nb_var; x++) {
        if (SEEN[x] == 0) {
            for (int s = 0; s < 2; s++) {
                assert(
                    (W->Head[x][s] == EOL) || ((S->State[x] != UNSET) && (S->State[x] & 1) == s));
            }
        }
    }
    return 1;
}

int check_sanity(formula_t* F, sol_t* S, watchlist_t* W, activelist_t* A)
{
    (void)F;
    (void)S;
    (void)W;
    (void)A;
    assert(check_sol(F, S));
    assert(check_watchlists(F, S, W));
    assert(check_activelist(F, S, W, A));
    return 1;
}

// vim600: set foldmethod=syntax textwidth=100:
