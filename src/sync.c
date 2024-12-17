#include "sync.h"
#include "search.h"
#include "utils_alpm.h"
#include "globals.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>

#define EXECVP(file, args, ...) do {\
    pid_t pid;\
    if ((pid=fork()) == 0) {\
        execvp(file, args);\
        perror("execvp");\
        exit(1);\
    } else if (pid < 0) {\
        perror("fork");\
    } else {\
        waitpid(pid, NULL, 0);\
    }} while (0)


int build_and_install(const int clone_dir_path_len,
                const char *clone_dir_path,
                const int makepkg_opts_len,
                char *const makepkg_opts[],
                const int sync_pkg_count,
                const char **sync_pkg_list);

int build_and_install_pkg(const int pkg_name_len,
                          const char* pkg_name,
                          const int clone_dir_path_len,
                          const char* clone_dir_path,
                          const int makepkg_opts_len,
                          char *const makepkg_opts[]);

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

    return build_and_install(clone_dir_path_len,
                       clone_dir_path,
                       makepkg_opts_len,
                       makepkg_opts,
                       sync_pkg_count,
                       sync_pkg_list);
}

// TODO:
//  - tell when something (PKGBUILD changes, etc) is changed from the aur
//
// NOTE: not accounting for when directory is bad (empty, deleted something in it, etc)
int build_and_install(const int clone_dir_path_len,
                const char *clone_dir_path,
                const int makepkg_opts_len,
                char *const makepkg_opts[],
                const int sync_pkg_count,
                const char **sync_pkg_list) {
    char *initial_cwd = getcwd(NULL, 0);

    for (int i = 0; i < sync_pkg_count; i++) {
        const char* pkg_name = sync_pkg_list[i];
        int ret = build_and_install_pkg(strlen(pkg_name),
                              pkg_name,
                              clone_dir_path_len,
                              clone_dir_path,
                              makepkg_opts_len,
                              makepkg_opts);
        if (ret != 0) {
            return ret;
        }
    }

    chdir(initial_cwd);
    free(initial_cwd);
    return 0;
}

// TODO: tell user about errors occurred
int build_and_install_pkg(const int pkg_name_len,
                          const char* pkg_name,
                          const int clone_dir_path_len,
                          const char* clone_dir_path,
                          const int makepkg_opts_len,
                          char *const makepkg_opts[]) {
    printf("Syncing %s\n", pkg_name);

    const int pkg_dir_path_len = clone_dir_path_len + 1 + pkg_name_len;
    char pkg_dir_path[pkg_dir_path_len + 1];
    memcpy(pkg_dir_path, clone_dir_path, clone_dir_path_len);
    pkg_dir_path[clone_dir_path_len] = '/';
    memcpy(pkg_dir_path + clone_dir_path_len + 1, pkg_name, pkg_name_len + 1);

    struct stat s;
    const int err = stat(pkg_dir_path, &s);
    bool git_pulled = false;
    if (err == -1) {
        if (errno == ENOENT) {
            const int aur_url_len = strlen(EXT_AUR_PKG_URL);
            const char *suffix = ".git";
            const int suffix_len = strlen(suffix);

            char url[aur_url_len + pkg_name_len + suffix_len + 1];
            int offset = 0;
            memcpy(url + offset, EXT_AUR_PKG_URL, aur_url_len);
            offset += aur_url_len;
            memcpy(url + offset, pkg_name, pkg_name_len);
            offset += pkg_name_len;
            memcpy(url + offset, suffix, suffix_len);
            offset += suffix_len;
            url[offset] = '\0';

            char *const git_args[] = {"git", "clone", url, NULL};
            chdir(clone_dir_path);

            printf(BOLD_GREEN "Cloning..." RCN);
            EXECVP("git", git_args);

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

    // WARN: Assuming system() succeeds
    //  hasn't handled all possible errors from system()
    if (!git_pulled) {
        printf(BOLD_GREEN "Pulling..." RCN);
        int ret = system("git pull");
        if (ret != 0) {
            printf(BOLD_RED "An error occurred while pulling repo." RCN);
            printf(BOLD_RED "Skipping %s" RCN, pkg_name);
            return 0;
        }
    }

    // TODO: get deps, make deps, handle aur deps
    // FILE *pipe = popen("source PKGBUILD; echo ${makedepends[*]} ${depends[*]}");
    // pclose(pipe);

    char *makepkg_args[1 + makepkg_opts_len + 1];
    makepkg_args[0] = "makepkg";
    memcpy(makepkg_args + 1, makepkg_opts, makepkg_opts_len * sizeof (char*));
    makepkg_args[makepkg_opts_len + 1] = NULL;

    printf(BOLD_GREEN "Running makepkg..." RCN);
    EXECVP("makepkg", makepkg_args);

    FILE *pipe = popen("makepkg --packagelist", "r");
    dyn_arr dyn_buf = dyn_arr_init(256, 0, sizeof (char), NULL);
    char buf[128];
    while (fgets(buf, sizeof buf, pipe) != NULL) {
        dyn_arr_append(&dyn_buf, sizeof buf, buf);
    }
    pclose(pipe);

    dyn_arr built_pkgs = dyn_arr_init(1, 0, sizeof (char*), NULL);
    for (size_t i = 0; i < dyn_buf.size && ((char*)dyn_buf.data)[i] != '\0'; i++) {
        int len = 0;
        int j = i;
        // shouldn't need to check for NUL since output always end in newline
        while (((char*)dyn_buf.data)[j] != '\n') {
            len++;
            j++;
        }
        char *pkg = malloc(len + 1);
        memcpy(pkg, dyn_buf.data, len);
        pkg[len] = '\0';
        dyn_arr_append(&built_pkgs, 1, &pkg);

        i += len;
    }

    // TODO: Install as dependencies or explicit
    for (size_t i = 0; i < built_pkgs.size; i++) {
        char *built_pkg = ((char**)built_pkgs.data)[i];
        char *sudo_args[] = {"sudo", "pacman", "-U", built_pkg, "--needed", NULL};
        EXECVP("sudo", sudo_args);
        free(built_pkg);
    }
    dyn_arr_free(&dyn_buf);
    return 0;
}
