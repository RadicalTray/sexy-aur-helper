#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "functions.h"
#include "types.h"
#include "globals.h"
#include "helper.h"

// TODO(fast): use chdir and refactor

// TODO: accept cd path, NULL = don't cd
int exec_sh_cmd(const char *cmd) {
    printf("Executing '%s'\n", cmd);
    return system(cmd);
}

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
}

// returns file contents and filesize of the package.txt
pkg_list_t get_aur_pkg_list() {
    FILE *p_file = fopen(g_pkg_list_filepath, "r");
    if (p_file == NULL) {
        fprintf(stderr, PKG_LIST_FILENAME " couldn't be opened!\n");
        return (pkg_list_t){ .size = 0, .buf = NULL };
    }

    struct stat pkg_list_filestat;
    stat(g_pkg_list_filepath, &pkg_list_filestat);

    printf("Hi\n");
    const int filesize = pkg_list_filestat.st_size;
    char *pkg_list = malloc(filesize);
    fread(&pkg_list, filesize, 1, p_file); // last char of pkg_list should be char '\n' or int = 10
    fclose(p_file);

    return (pkg_list_t){ .size = filesize, .buf = pkg_list, };
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

    const pkg_list_t pkg_list = get_aur_pkg_list();
    if (pkg_list.buf == NULL) {
        return 1;
    }

    int matched_pkgs_count;
    char **matched_pkgs;
    search_pkg(len, args, pkg_list.size, pkg_list.buf, &matched_pkgs_count, &matched_pkgs);
    free(pkg_list.buf);

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

// TODO:
//  - don't use system() for running makepkg (user can be stupid with options)
//  - tell when something (PKGBUILD changes, etc) is changed from the aur
//
// NOTE: not accounting for when directory is bad (empty, deleted something in it, etc)
int run_makepkg(const int clone_dir_path_len,
                const char *clone_dir_path,
                const char *makepkg_opts,
                const int sync_pkg_count,
                const char **sync_pkg_list) {
    // should have just used paths with '/' suffix bruh
    const int ext_clone_dir_path_len = clone_dir_path_len + 1; // + 1 for '/'
    char ext_clone_dir_path[ext_clone_dir_path_len + 1];

    memcpy(ext_clone_dir_path, clone_dir_path, clone_dir_path_len);
    ext_clone_dir_path[ext_clone_dir_path_len - 1] = '/';
    ext_clone_dir_path[ext_clone_dir_path_len] = '\0';

    for (int i = 0; i < sync_pkg_count; i++) {
        const char *pkg_name = sync_pkg_list[i];

        printf("Syncing %s\n", pkg_name);

        // WARN: will paths, url always be valid?
        // dupe code
        const char *clone_sh_cmds[6] = {
            "cd '",
            clone_dir_path,
            "'; git clone ",
            EXT_AUR_PKG_URL,
            pkg_name,
            ".git",
        };
        int clone_sh_cmd_lens[6], clone_cmd_len = 0;
        for (int i = 0; i < 6; i++) {
            clone_sh_cmd_lens[i] = strlen(clone_sh_cmds[i]);
            clone_cmd_len += clone_sh_cmd_lens[i];
        }

        char clone_cmd[clone_cmd_len + 1];
        int clone_offset = 0;
        for (int i = 0; i < 6; i++) {
            memcpy(clone_cmd + clone_offset, clone_sh_cmds[i], clone_sh_cmd_lens[i]);
            clone_offset += clone_sh_cmd_lens[i];
        }
        clone_cmd[clone_cmd_len] = '\0';

        const int pkg_name_len = clone_sh_cmd_lens[4];
        const int pkg_dir_path_len = ext_clone_dir_path_len + pkg_name_len;
        char pkg_dir_path[pkg_dir_path_len + 1];
        memcpy(pkg_dir_path, ext_clone_dir_path, ext_clone_dir_path_len);
        memcpy(pkg_dir_path + ext_clone_dir_path_len, pkg_name, pkg_name_len + 1);

        struct stat s;
        const int err = stat(pkg_dir_path, &s);
        bool git_pulled = false;
        if (err == -1) {
            if(errno == ENOENT) {
                exec_sh_cmd(clone_cmd);
                git_pulled = true;
            } else {
                perror("stat");
                return 1;
            }
        } else {
            if (!S_ISDIR(s.st_mode)) {
                fprintf(stderr, "%s already exists, but is not a directory!", pkg_dir_path);
                return 1;
            }
        }

        if (!git_pulled) {
            const char *pull_sh_cmds[3] = {
                "cd '",
                pkg_dir_path,
                "'; git pull",
            };
            int pull_sh_cmd_lens[3], pull_cmd_len = 0;
            for (int i = 0; i < 3; i++) {
                pull_sh_cmd_lens[i] = strlen(pull_sh_cmds[i]);
                pull_cmd_len += pull_sh_cmd_lens[i];
            }

            char pull_cmd[pull_cmd_len + 1];
            int pull_offset = 0;
            for (int i = 0; i < 3; i++) {
                memcpy(pull_cmd + pull_offset, pull_sh_cmds[i], pull_sh_cmd_lens[i]);
                pull_offset += pull_sh_cmd_lens[i];
            }
            pull_cmd[pull_cmd_len] = '\0';

            exec_sh_cmd(pull_cmd);
        }

        // dupe code
        const char *makepkg_sh_cmds[4] = {
            "cd '",
            pkg_dir_path,
            "'; makepkg ",
            makepkg_opts,
        };
        int makepkg_sh_cmd_lens[4], makepkg_cmd_len = 0;
        for (int i = 0; i < 4; i++) {
            makepkg_sh_cmd_lens[i] = strlen(makepkg_sh_cmds[i]);
            makepkg_cmd_len += makepkg_sh_cmd_lens[i];
        }

        char makepkg_cmd[makepkg_cmd_len + 1];
        int makepkg_offset = 0;
        for (int i = 0; i < 4; i++) {
            memcpy(makepkg_cmd + makepkg_offset, makepkg_sh_cmds[i], makepkg_sh_cmd_lens[i]);
            makepkg_offset += makepkg_sh_cmd_lens[i];
        }
        makepkg_cmd[makepkg_cmd_len] = '\0';

        exec_sh_cmd(makepkg_cmd);
    }
    return 0;
}

// TODO: --options='str'
int run_sync(const int len, const char **args) {
    const char *makepkg_opts = "-si";
    int sync_pkg_count = 0;
    bool is_pkg[len] = {}; // hopefully initialized to false
    for (int i = 0; i < len; i++) {
        if (strcmp(args[i], "-o") == 0 ||
            strcmp(args[i], "--options") == 0) {
            if (i + 1 == len) {
                fprintf(stderr, "Specify options for makepkg.\n");
                return 1;
            }
            i++;
            makepkg_opts = args[i];
            continue;
        }

        sync_pkg_count++;
        is_pkg[i] = true;
    }

    if (sync_pkg_count == 0) {
        fprintf(stderr, "Specify packages to sync.\n");
        return 1;
    }

    const char *sync_pkg_list[sync_pkg_count];
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (is_pkg[i]) {
            sync_pkg_list[j] = args[i];
            j++;
            if (j == sync_pkg_count) {
                break;
            }
        }
    }

    const pkg_list_t pkg_list = get_aur_pkg_list();
    if (pkg_list.buf == NULL) {
        return 1;
    }

    bool error = false;
    for (int i = 0; i < sync_pkg_count; i++) {
        if (check_pkg(strlen(sync_pkg_list[i]), sync_pkg_list[i], pkg_list.size, pkg_list.buf) == false) {
            fprintf(stderr, "'%s' not found.\n", sync_pkg_list[i]);
            error = true;
        }
    }
    free(pkg_list.buf);
    if (error) {
        return 1;
    }

    const int cache_dir_len = strlen(g_cache_dir);
    const char *clone_dir = "/clone";
    const int clone_dir_len = strlen(clone_dir);

    const int clone_dir_path_len = cache_dir_len + clone_dir_len;
    char clone_dir_path[clone_dir_path_len + 1];
    memcpy(clone_dir_path, g_cache_dir, cache_dir_len);
    memcpy(clone_dir_path + cache_dir_len, clone_dir, clone_dir_len + 1);

    mkdir(clone_dir_path, S_IRWXU);
    if (errno != -1 && errno != EEXIST) {
        perror("mkdir");
    }

    return run_makepkg(clone_dir_path_len, clone_dir_path, makepkg_opts, sync_pkg_count, sync_pkg_list);
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
    if (len != 0) {
        fprintf(stderr, "Arguments unexpected: ");
        for (int i = 0; i < len - 1; i++) {
            fprintf(stderr, "'%s' ", args[i]);
        }
        fprintf(stderr, "'%s'\n", args[len - 1]);
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
