#include "utils.h"
#include "types.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: accept cd path, NULL = don't cd
int exec_sh_cmd(const char *cmd) {
    printf("Executing '%s'\n", cmd);
    return system(cmd);
}

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
}

int set_globals() {
    const char *cache_home = getenv(XDG_CACHE_HOME);
    if (strlen(cache_home) <= 0) {
        fprintf(stderr,"Empty " "$" XDG_CACHE_HOME "\n");
        return 1;
    }
    g_cache_dir = str_concat(cache_home, "/" PROGRAM_NAME);
    g_pkg_list_filepath = str_concat(g_cache_dir, "/" PKG_LIST_FILENAME);
    return 0;
}

char* str_concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *str = malloc(len1 + len2 + 1);
    memcpy(str, s1, len1);
    memcpy(str + len1, s2, len2 + 1);
    return str;
}
