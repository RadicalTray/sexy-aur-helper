#include "search.h"
#include "types.h"
#include "update.h"
#include "utils.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

int run_search(const int len, const char **args) {
    if (len == 0) {
        print_help(stderr);
        return 1;
    }

    const aur_pkg_list_t *pkg_list = get_aur_pkg_list();
    if (pkg_list == NULL) {
        fprintf(stderr, "get_aur_pkg_list() failed\n");
        return 1;
    }

    int matched_pkgs_count;
    char **matched_pkgs;
    search_pkg(len, args, pkg_list->size, pkg_list->buf, &matched_pkgs_count, &matched_pkgs);

    printf("matched_pkgs_count: %i\n", matched_pkgs_count);
    for (int i = 0; i < matched_pkgs_count; i++) {
        printf("%s\n", matched_pkgs[i]);
        free(matched_pkgs[i]);
    }
    free(matched_pkgs);
    return 0;
}

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

// smartass again
int pkg_is_in_aur(const int sync_pkg_name_len, const char *sync_pkg_name) {
    const aur_pkg_list_t *pkg_list = get_aur_pkg_list();
    if (pkg_list == NULL) {
        fprintf(stderr, "get_aur_pkg_list() failed");
        return 69;
    }
    const int pkg_list_size = pkg_list->size;
    const char *pkg_list_buf = pkg_list->buf;

    for (int i = 0; i < pkg_list_size; i++) {
        int pkg_name_len = 0;
        int matched_char_count = 0;
        while (pkg_list_buf[i + pkg_name_len] != '\n') {
            matched_char_count += pkg_name_len < sync_pkg_name_len &&
                sync_pkg_name[pkg_name_len] == pkg_list_buf[i + pkg_name_len];
            pkg_name_len++;
        }

        if (sync_pkg_name_len == pkg_name_len &&
            matched_char_count == sync_pkg_name_len) {
            return 0;
        }

        i += pkg_name_len; // don't forget to move i to '\n'
    }

    return 1;
}

