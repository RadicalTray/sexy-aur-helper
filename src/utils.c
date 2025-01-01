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
    fprintf(fptr,
            "Search the AUR package list:\n"
            "\tsaur search <search string>\n"
            "Sync a package or multiple packages:\n"
            "\tsaur sync <package name> [package name] ....\n"
            "Upgrade system and AUR packages:\n"
            "\tsaur upgrade\n"
            "Update the AUR package list:\n"
            "\tsaur update-pkg-list\n"
    );
}

int set_globals() {
    char *cache_home;
    const char *xdg_cache_home = getenv(XDG_CACHE_HOME);
    if (xdg_cache_home == NULL) {
        fprintf(stderr, BOLD_RED "$" XDG_CACHE_HOME " is not set!" RCN);

        const char *user_home = getenv("HOME");
        if (user_home == NULL) {
            fprintf(stderr, BOLD_RED "$HOME is not set! WHAT DID YOU DO?" RCN);
            return 1;
        }

        cache_home = str_concat(user_home, "/.config");
        fprintf(stderr, BOLD_RED "Defaulting to %s" RCN, cache_home);
    } else {
        int xdg_cache_home_len = strlen(xdg_cache_home);
        cache_home = malloc(xdg_cache_home_len + 1);
        strcpy(cache_home, xdg_cache_home);
    }

    g_cache_dir = str_concat(cache_home, "/" PROGRAM_NAME);
    free(cache_home);

    g_pkg_list_filepath = str_concat(g_cache_dir, "/" PKG_LIST_FILENAME);
    g_pkgbase_list_filepath = str_concat(g_cache_dir, "/" PKGBASE_LIST_FILENAME);

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
