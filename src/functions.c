#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"
#include "types.h"
#include "globals.h"
#include "helper.h"

int exec_sh_cmd(const char *cmd) {
    printf("Executing '%s'\n", cmd);
    return system(cmd);
}

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
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

int run_search(const int len, const char **args) {
    if (len == 0) {
        print_help(stderr);
        return 1;
    }

    if (access(g_pkg_list_filepath, F_OK) != 0) {
        if (run_update_pkg_list(0, NULL) != 0) {
            fprintf(stderr, "Couldn't fetch the package list!\n");
            return 1;
        }
    }

    // dupe code
    FILE *p_file = fopen(g_pkg_list_filepath, "r");
    if (p_file == NULL) {
        fprintf(stderr, PKG_LIST_FILENAME " couldn't be opened!\n");
        return 1;
    }

    struct stat pkg_list_filestat;
    stat(g_pkg_list_filepath, &pkg_list_filestat);

    const int filesize = pkg_list_filestat.st_size;
    char pkg_list[filesize];
    fread(&pkg_list, filesize, 1, p_file); // last char of pkg_list should be char '\n' or int = 10
    fclose(p_file);

    int matched_pkgs_count;
    char **matched_pkgs;
    search_pkg(len, args, pkg_list_filestat.st_size, pkg_list, &matched_pkgs_count, &matched_pkgs);

    printf("matched_pkgs_count: %i\n", matched_pkgs_count);
    for (int i = 0; i < matched_pkgs_count; i++) {
        printf("%s\n", matched_pkgs[i]);
        free(matched_pkgs[i]);
    }
    free(matched_pkgs);
    return 0;
}

// smartass again
//
// TODO: impl show multiple pkg versions (pkg, pkg-git, pkg-*)
bool check_pkg(const int sync_pkg_name_len,
              const char *sync_pkg_name,
              const int pkg_list_size,
              const char *pkg_list) {
    for (int i = 0; i < pkg_list_size; i++) {
        int pkg_name_len = 0;
        int matched_char_count = 0;
        while (pkg_list[i + pkg_name_len] != '\n') {
            matched_char_count += pkg_name_len < sync_pkg_name_len &&
                sync_pkg_name[pkg_name_len] == pkg_list[i + pkg_name_len];
            pkg_name_len++;
        }

        // NOTE: the multiple versions case will have
        //          matched_char_count == pkg_name_len?
        // TODO: maybe impl that later
        if (sync_pkg_name_len == pkg_name_len &&
            matched_char_count == sync_pkg_name_len) {
            return true;
        }

        i += pkg_name_len; // don't forget to move i to '\n'
    }
    return false;
}

void run_makepkg(const int sync_pkg_count, const char **sync_pkg_list) {
}

// TODO: --options='str'
int run_sync(const int len, const char **args) {
    const char *makepkg_opts = "-si";
    int sync_pkg_count = 0;
    bool indices[len] = {}; // hopefully initialized to false
    for (int i = 0; i < len; i++) {
        if (strcmp(args[i], "-o") == 0 ||
            strcmp(args[i], "--options") == 0) {
            i++;
            if (i == len) {
                fprintf(stderr, "Specify options for '%s'\n", args[i]);
                return 1;
            }
            makepkg_opts = args[i];
            continue;
        }

        sync_pkg_count++;
        indices[i] = true;
    }

    if (sync_pkg_count == 0) {
        fprintf(stderr, "Specify packages to sync.\n");
        return 1;
    }

    const char *sync_pkg_list[sync_pkg_count];
    int x = 0;
    for (int i = 0; i < len; i++) {
        if (indices[i] == true) {
            sync_pkg_list[x] = args[i];
            x++;
        }
    }

    // dupe code
    FILE *p_file = fopen(g_pkg_list_filepath, "r");
    if (p_file == NULL) {
        fprintf(stderr, PKG_LIST_FILENAME " couldn't be opened!\n");
        return 1;
    }

    struct stat pkg_list_filestat;
    stat(g_pkg_list_filepath, &pkg_list_filestat);

    const int filesize = pkg_list_filestat.st_size;
    char pkg_list[filesize];
    fread(&pkg_list, filesize, 1, p_file); // last char of pkg_list should be char '\n' or int = 10
    fclose(p_file);

    bool error = false;
    for (int i = 0; i < sync_pkg_count; i++) {
        if(check_pkg(strlen(sync_pkg_list[i]), sync_pkg_list[i], filesize, pkg_list) == false) {
            fprintf(stderr, "'%s' not found.\n", sync_pkg_list[i]);
            error = true;
        }
    }
    if (error) {
        return 1;
    }

    run_makepkg(sync_pkg_count, sync_pkg_list);
    return 0;
}

int run_upgrade(const int len, const char **args) {
    if (len != 0) {
        printf("Individual pkg upgrade is not implemented.\n");
        print_help(stderr);
        return 1;
    }

    return 0;
}

int run_update_pkg_list(const int len, const char **args) {
    if (len != 0 || args == NULL) {
        print_help(stderr);
        return 1;
    }

    // smartass
    const char *sh_cmd = "curl https://aur.archlinux.org/packages.gz | gzip -cd > ";
    const int sh_cmd_len = strlen(sh_cmd);
    const int filepath_len = strlen(g_pkg_list_filepath);
    const int cmd_len = sh_cmd_len + filepath_len;
    char cmd[cmd_len + 1]; // This should allocate it on the stack right?
    memcpy(cmd, sh_cmd, sh_cmd_len);
    memcpy(cmd + sh_cmd_len, g_pkg_list_filepath, filepath_len + 1);

    if (exec_sh_cmd(cmd) != 0) {
        fprintf(stderr, "An error happended while trying to fetch the package list!\n");
        return 1;
    }
    return 0;
}

int run_clear_cache(const int len, const char **args) {
    if (len != 0 || args == NULL) {
        print_help(stderr);
        return 1;
    }

    return 0;
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

// this is definitely overkill lol
void cleanup() {
    free(g_cache_dir);
    free(g_pkg_list_filepath);
}
