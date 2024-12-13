#include "search.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
// I might have been a smartass here.
//
// another way is to return indices of pkgs in pkg_list, but pkg_list is not an array
// something would have to change, idk if that's also better
//
// TODO(when i rewrite this in rust i guess (i give up)): impl fuzzy search
//
// pkg_list isn't a c string.
void search_pkg(const int search_strslen,
                const char **search_strs,
                const int pkg_list_size,
                const char *pkg_list,
                int *dst_matched_pkgs_count,
                char ***dst_matched_pkgs) {
    int matched_pkgs_count = 0;
    // fuck it im hardcoding it, i'm not making a dynamic array
    const int matched_pkgs_max_count = 50;
    char **matched_pkgs = malloc(matched_pkgs_max_count * sizeof(char*));
    for (int i = 0; i < pkg_list_size && matched_pkgs_count < matched_pkgs_max_count; i++) {
        int pkg_strlen = 0;
        while (pkg_list[i + pkg_strlen] != '\n') {
            pkg_strlen++;
        }

        char *pkg_name = malloc(pkg_strlen + 1); // +1 for NUL
        for (int j = 0; j < pkg_strlen; j++) {
            pkg_name[j] = pkg_list[i + j];
        }
        pkg_name[pkg_strlen] = '\0';

        i += pkg_strlen; // don't forget to move i to '\n'

        bool isMatch = false;
        for (int j = 0; j < search_strslen && matched_pkgs_count < matched_pkgs_max_count; j++) {
            if (strstr(pkg_name, search_strs[j]) != NULL) {
                matched_pkgs[matched_pkgs_count] = pkg_name;
                matched_pkgs_count++;
                isMatch = true;
                break;
            }
        }
        if (isMatch) {
            continue;
        }

        free(pkg_name);
    }

    *dst_matched_pkgs_count = matched_pkgs_count;
    *dst_matched_pkgs = matched_pkgs;
}
