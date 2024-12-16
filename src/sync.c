#include "sync.h"
#include "search.h"
#include "utils_alpm.h"
#include "globals.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>

// TODO:
//  - tell when something (PKGBUILD changes, etc) is changed from the aur
//
// TODO: abstract execvp
//
// NOTE: not accounting for when directory is bad (empty, deleted something in it, etc)
int run_makepkg(const int clone_dir_path_len,
                const char *clone_dir_path,
                const int makepkg_opts_len,
                char *const makepkg_opts[],
                const int sync_pkg_count,
                const char **sync_pkg_list) {
    // TODO: is this really needed?
    const int ext_clone_dir_path_len = clone_dir_path_len + 1; // + 1 for '/'
    char ext_clone_dir_path[ext_clone_dir_path_len + 1];

    memcpy(ext_clone_dir_path, clone_dir_path, clone_dir_path_len);
    ext_clone_dir_path[ext_clone_dir_path_len - 1] = '/';
    ext_clone_dir_path[ext_clone_dir_path_len] = '\0';

    char *initial_cwd = getcwd(NULL, 0);

    const int aur_url_len = strlen(EXT_AUR_PKG_URL);
    const char *suffix = ".git";
    const int suffix_len = strlen(suffix);

    for (int i = 0; i < sync_pkg_count; i++) {
        const char *pkg_name = sync_pkg_list[i];
        const int pkg_name_len = strlen(pkg_name);

        printf("Syncing %s\n", pkg_name);

        const int pkg_dir_path_len = ext_clone_dir_path_len + pkg_name_len;
        char pkg_dir_path[pkg_dir_path_len + 1];
        memcpy(pkg_dir_path, ext_clone_dir_path, ext_clone_dir_path_len);
        memcpy(pkg_dir_path + ext_clone_dir_path_len, pkg_name, pkg_name_len + 1);

        struct stat s;
        const int err = stat(pkg_dir_path, &s);
        bool git_pulled = false;
        if (err == -1) {
            if (errno == ENOENT) {
                char url[aur_url_len + pkg_name_len + suffix_len + 1];
                int offset = 0;
                memcpy(url + offset, EXT_AUR_PKG_URL, aur_url_len);
                offset += aur_url_len;
                memcpy(url + offset, pkg_name, pkg_name_len);
                offset += pkg_name_len;
                memcpy(url + offset, suffix, suffix_len);
                offset += suffix_len;
                url[offset] = '\0';

                char *const args[] = {"git", "clone", url, NULL};
                chdir(clone_dir_path);

                pid_t pid;
                if ((pid=fork()) == 0) {
                    printf(BOLD_GREEN "Cloning..." RCN);
                    execvp("git", args);
                    perror("execvp");
                    exit(1);
                } else if (pid < 0) {
                    perror("fork");
                } else {
                    waitpid(pid, NULL, 0);
                }

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

        chdir(pkg_dir_path);

        if (!git_pulled) {
            char *const args[] = {"git", "pull", NULL};
            pid_t pid;
            if ((pid=fork()) == 0) {
                printf(BOLD_GREEN "Pulling..." RCN);
                execvp("git", args);
                perror("execvp");
                exit(1);
            } else if (pid < 0) {
                perror("fork");
            } else {
                waitpid(pid, NULL, 0);
            }
        }

        char *args[1 + makepkg_opts_len + 1];
        args[0] = "makepkg";
        memcpy(args + 1, makepkg_opts, makepkg_opts_len * sizeof(char*));
        args[makepkg_opts_len + 1] = NULL;

        pid_t pid;
        if ((pid=fork()) == 0) {
            printf(BOLD_GREEN "Running makepkg..." RCN);
            execvp("makepkg", args);
            perror("execvp");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
        } else {
            waitpid(pid, NULL, 0);
        }
    }

    chdir(initial_cwd);
    free(initial_cwd);
    return 0;
}

int sync_pkg(const int sync_pkg_count, const char **sync_pkg_list, const int makepkg_opts_len, char *const makepkg_opts[]) {
    bool error = false;
    for (int i = 0; i < sync_pkg_count; i++) {
        int ret = pkg_is_in_aur(strlen(sync_pkg_list[i]), sync_pkg_list[i]);
        // could've used if else but nvm
        switch (ret) {
            case 1: {
                fprintf(stdout, "'%s' not found in aur package list.\n", sync_pkg_list[i]);
                error = true;
                break;
            } case 69: {
                error = true;
                break;
            }
        }
    }
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

    int mkdir_err = mkdir(clone_dir_path, S_IRWXU);
    if (mkdir_err != 0 && errno != EEXIST) {
        perror("mkdir");
    }

    return run_makepkg(clone_dir_path_len,
                       clone_dir_path,
                       makepkg_opts_len,
                       makepkg_opts,
                       sync_pkg_count,
                       sync_pkg_list);
}

int run_sync(const int len, const char **args) {
    const char *makepkg_opts_str = "";
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
            makepkg_opts_str = args[i];
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

    const int makepkg_opts_str_len = strlen(makepkg_opts_str);
    int makepkg_opts_len = 0;

    // check for number of args
    bool after_whitespace = true;
    for (int i = 0; i < makepkg_opts_str_len; i++) {
        if (makepkg_opts_str[i] == ' ') {
            after_whitespace = true;
            continue;
        }
        if (after_whitespace) {
            makepkg_opts_len++;
            after_whitespace = false;
        }
    }

    char *makepkg_opts[makepkg_opts_len];
    int current_opt_idx = 0;
    for (int i = 0; i < makepkg_opts_str_len; i++) {
        if (current_opt_idx == makepkg_opts_len) {
            break;
        }

        if (makepkg_opts_str[i] == ' ') {
            continue;
        }

        int opt_strlen = 0;
        for (int j = i; j < makepkg_opts_str_len; j++) {
            if (makepkg_opts_str[j] == ' ') {
                break;
            }

            opt_strlen++;
        }

        char *opt_str = malloc(opt_strlen + 1);
        memcpy(opt_str, makepkg_opts_str + i, opt_strlen);
        opt_str[opt_strlen] = '\0';

        makepkg_opts[current_opt_idx] = opt_str;

        i += opt_strlen;
        current_opt_idx++;
    }

    int ret = sync_pkg(sync_pkg_count, sync_pkg_list, makepkg_opts_len, makepkg_opts);
    for (int i = 0; i < makepkg_opts_len; i++) {
        free(makepkg_opts[i]);
    }
    return ret;
}
