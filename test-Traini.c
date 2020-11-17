#include "sat.h"

void help_test()
{
    printf("help                    list available tests\n"
           "TRUC [FICHIER]          un p'tit test qui ne fait rien\n"
           "struct [FICHIER]        show representation of CNF formula\n"
           "CNF [FICHIER]           show 'human readable' CNF formula\n"
           "pCNF [FICHIER]          show pretty CNF formula\n"
           "watchlists [FICHIER]    show watchlists for the CNF formula\n"
           "active [FICHIER]        show active lists for the CNF formula\n"
           "\n");
}

int test(char* cmd, int argc, char** argv)
{

    if (0 == strcmp(cmd, "help")) {
        help_test();
        return 0;
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
    printf("The formula contains %d variable(s), %d clause(s) for a total of %d literal(s)\n",
        F->nb_var, F->nb_cl, F->nb_lit);

    if (0 == strcmp(cmd, "TRUC")) {
        printf("TEST => mon test à moi...\n");
        // F contient la formule CNF lue dans un fichier (ou sur l'entrée standard)
        // TODO
    } else if (0 == strcmp(cmd, "struct")) {
        printf("TEST => show formula's arrays\n");
        print_struct_formula(F);
    } else if (0 == strcmp(cmd, "CNF")) {
        printf("TEST => show CNF formula\n");
        print_CNF(F);
    } else if (0 == strcmp(cmd, "pCNF")) {
        printf("TEST => pretty print CNF formula\n");
        pprint_CNF(F);
    } else if (0 == strcmp(cmd, "watchlists")) {
        printf("TEST => show watch lists\n");
        watchlist_t* W = init_watchlists(F);
        pprint_watchlists(F, W);
    } else if (0 == strcmp(cmd, "active")) {
        printf("TEST => show active list\n");
        watchlist_t* W = init_watchlists(F);
        activelist_t* A = init_activelist(F, W);
        pprint_activelist(A);
    } else {
        printf("unknown test: '%s'\n", cmd);
    }

    return 0;
}

// vim600: set foldmethod=syntax textwidth=100:
