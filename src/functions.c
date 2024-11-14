#include "stdio.h"
#include "functions.h"
#include "types.h"
#include "globals.h"

Opts process_args(const int len, char **args) {
    Opts opts = g_default_opts;
    for (int i = 0; i < len; i++) {
        for (const char *p_char = args[i]; *p_char != 0; p_char++) {
        }
    }
    return opts;
}

void process_opt() {
}

void process_long_opt() {
}

void print_help() {
    printf("HELP\n");
}

int run_search(const int len, const char **args) {
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
