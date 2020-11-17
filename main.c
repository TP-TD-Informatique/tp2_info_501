#include "sat.h"

int VERBOSE = 0;
int BUF_SIZE = 4096;

void help(char* exec)
{
    printf("usage: %s [options]\n"
           "reads a DIMACS file from stdin and tries to satisfies the "
           "corresponding DNF formula\n"
           "\n"
           "options:\n"
           "  -h  /  --help             this message\n"
           "  -b  /  --buf_size         set buffer size to read input lines (default: 4096)\n"
           //
           "  -v  /  --verbose          increase verbosity of debug messages\n"
           "  -q  /  --quiet            do not print the solution\n"
           //
           "  -N  /  --naive            use the naive algorithm\n"
           "  -W  /  --watchlist        use watchlists for the naive algorithm\n"
           "  -A  /  --activelist       use watchlists and an active list of variables\n"
           "  -D  /  --DPLL             use watchlists, an active list, and the DPLL algorithm\n"
           "  -P  /  --preprocess       preprocess the formula by removing unit clauses\n"
           "  -X  /  --negate           print negation of solution, in DIMACS format\n"
           "  -T TEST  /  --test=TEST   call the test function\n",
        exec);
}

// possible algorithms
#define NAIVE 0
#define WATCH 1
#define ACTIVE 2
#define DPLL 3
#define TESTS 9

int result(formula_t* F, sol_t* S, int quiet, int invert, int sat)
{
    if (sat) {
        if (invert != 1) {
            printf("SATISFIABLE\n");
            if (!quiet)
                print_final_solution(F, S);
        } else {
            if (!quiet)
                print_negated_solution(NULL, S);
        }
        if (!is_solution(F, S)) {
            fprintf(stderr, "*** IS THAT REALLY A SOLUTION?\n");
        }
        free_formula(F);
        free_sol(S);
        return 0;
    } else {
        if (invert != 1) {
            printf("UNSATISFIABLE\n");
        } else {
            printf("c UNSATISFIABLE\n");
        }
        free_formula(F);
        free_sol(S);
        return 1;
    }
}

int main(int argc, char* argv[])
{
    char short_options[] = "hb:vqT:NWADXP";
    static struct option long_options[] = { { "help", no_argument, 0, 'h' },
        { "buf_size", required_argument, 0, 'b' }, { "verbose", no_argument, 0, 'v' },
        { "naive", no_argument, 0, 'N' }, { "watchlist", no_argument, 0, 'W' },
        { "activelist", no_argument, 0, 'A' }, { "DPLL", no_argument, 0, 'D' },
        { "quiet", no_argument, 0, 'q' }, { "preprocess", no_argument, 0, 'P' },
        { "negate", no_argument, 0, 'X' }, { "test", no_argument, 0, 't' }, { 0, 0, 0, 0 } };

    int opt;
    int long_index;
    int algorithm = DPLL;
    char test_cmd[32] = "";
    int invert = 0;
    int quiet = 0;
    int preproc = 0;

    while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
        switch (opt) {
        case 'N':
            algorithm = NAIVE;
            break;
        case 'W':
            algorithm = WATCH;
            break;
        case 'A':
            algorithm = ACTIVE;
            break;
        case 'D':
            algorithm = DPLL;
            break;
        case 'P':
            preproc = 1;
            break;
        case 'T':
            algorithm = TESTS;
            strncpy(test_cmd, optarg, 32);
            test_cmd[31] = '\0';
            break;
        case 'X':
            invert = 1;
            break;
        case 'q':
            quiet = 1;
            break;

        case 'v':
            VERBOSE++;
            break;
        case 'b':
            BUF_SIZE = atoi(optarg);
            break;

        case 'h':
            help(argv[0]);
            return 0;
        default: /* '?' */
            // TODO
            return 1;
        }
    }
    argc -= optind;
    argv += optind;

    int sat;

    if (algorithm == TESTS) {
        return test(test_cmd, argc, argv);
    }

    FILE* f_in;
    if (argc > 0) {
        if (NULL == (f_in = fopen(argv[0], "r"))) {
            fprintf(stderr, "*** error opening file %s: %s\n", argv[0], strerror(errno));
            exit(5);
        }
    } else {
        f_in = stdin;
    }
    formula_t* F = parse_formula(f_in);
    LOG(1, "The formula contains %d variable(s), %d clause(s) for a total of %d literal(s)\n",
        F->nb_var, F->nb_cl, F->nb_lit);

    sol_t* S = new_sol(F->nb_var);

    if (preproc) {
        if (algorithm == NAIVE || algorithm == WATCH) {
            fprintf(stderr, "*** Can only preprocess formulas when using active lists...\n");
        } else {
            LOG(1, "preprocessing formula...\n");
            int r = preprocess(F, S);
            if (r == -1) {
                return result(F, S, quiet, invert, 0);
            }
            if (r == 1) {
                return result(F, S, quiet, invert, 1);
            }
            LOG(1,
                "The new formula contains %d variable(s), %d clause(s) for a total of %d "
                "literal(s)\n",
                F->nb_var, F->nb_cl, F->nb_lit);
        }
    }
    watchlist_t* W = NULL;
    activelist_t* A = NULL;
    int BCP = 0;

    if (algorithm == NAIVE) {
        // nothing to do
    } else if (algorithm == WATCH) {
        W = init_watchlists(F);
    } else if (algorithm == ACTIVE) {
        W = init_watchlists(F);
        A = init_activelist(F, W);
    } else if (algorithm == DPLL) {
        W = init_watchlists(F);
        A = init_activelist(F, W);
        BCP = 1;
    } else {
        fprintf(stderr, "BUG, this shouldn't happen\n");
        exit(7);
    }

    if (W == NULL) {
        sat = solve_naive(F, S);
    } else {
        sat = solve(F, S, W, A, BCP);
    }
    free_watchlist(W);
    free_activelist(A);

    return result(F, S, quiet, invert, sat);
}

// vim600: set foldmethod=syntax textwidth=100:
