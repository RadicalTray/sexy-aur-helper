#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "types.h"
#include "globals.h"
#include "helper.h"

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
}

int run_search(const int len, const char **args) {
    if (access(PATH(PKG_LIST_FILENAME), F_OK) != 0) {
        if (run_update_pkg_list(0, NULL) != 0) {
            fprintf(stderr, "Couldn't fetch the package list!");
            return 1;
        }
    }
    return 0;
}

int run_sync(const int len, const char **args) {
    return 0;
}

int run_upgrade(const int len, const char **args) {
    return 0;
}

int run_update_pkg_list(const int len, const char **args) {
    if (len != 0) {
        print_help(stderr);
        return 1;
    }

    const char *sh_cmd = "curl https://aur.archlinux.org/packages.gz | gzip -cd > ";
    const char *filepath = str_concat(g_cache_dir, "/" PKG_LIST_FILENAME);
    sh_cmd = str_concat(sh_cmd, filepath);

    printf("Executing '%s'\n", sh_cmd);
    if (system(sh_cmd) != 0) {
        fprintf(stderr, "Error happended while trying to fetch the package list");
        return 1;
    }
    return 0;
}

int run_clear_cache(const int len, const char **args) {
    return 0;
}

int set_globals() {
    const char *cache_home = getenv(XDG_CACHE_HOME);
    if (strlen(cache_home) <= 0) {
        fprintf(stderr,"Empty " "$" XDG_CACHE_HOME "\n");
        return 1;
    }
    g_cache_dir = str_concat(cache_home, "/" PROGRAM_NAME);
    return 0;
}
