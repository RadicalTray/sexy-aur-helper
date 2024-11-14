#include <stdlib.h>
#include <stdio.h>
#include "functions.h"
#include "types.h"
#include "globals.h"
#include "helper.h"

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
}

int run_search(const int len, const char **args) {
    system("echo hello");
    PATH("packages.txt");
    return 0;
}

int run_sync(const int len, const char **args) {
    return 0;
}

int run_upgrade(const int len, const char **args) {
    return 0;
}

int run_update_pkg_list(const int len, const char **args) {
    return 0;
}

int run_clear_cache(const int len, const char **args) {
    return 0;
}

int set_globals() {
    const char *cache_home = getenv(XDG_CACHE_HOME);
    g_cache_dir = str_concat(cache_home, "/" PROGRAM_NAME);
    return 0;
}
