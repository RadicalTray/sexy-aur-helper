#include "cache.h"
#include "utils.h"
#include <stdio.h>

int run_clear_cache(const int len, const char **args) {
    if (len != 0) {
        fprintf(stderr, "Arguments unexpected: ");
        for (int i = 0; i < len - 1; i++) {
            fprintf(stderr, "'%s' ", args[i]);
        }
        fprintf(stderr, "'%s'\n", args[len - 1]);
        print_help(stderr);
        return 1;
    }

    return 0;
}
