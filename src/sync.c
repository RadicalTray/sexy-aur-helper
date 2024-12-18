// WARN: haven't handle waitpid errors
// WARN: Assuming system() succeeds

// TODO: fetch every package -> makepkg every pkg -> install all
// TODO: Install as dependencies or explicit
// TODO: Check for changes in git repo
// TODO: Download all packages at the end

#include "sync.h"
#include "search.h"
#include "utils.h"
#include "utils_alpm.h"
#include "globals.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>


typedef struct {
    char *pkg_name;
    char *err_msg;
} error_t;


typedef enum {
    NO_CHANGES,
    NEW,
    UPDATE,
} e_diff_type;

typedef struct {
    char *pkg_name;
    char *pkg_dir_path;
    char *built_pkg_path;
    e_diff_type diff;
} pkginfo_t;


#define EXECVP(file, args, ...) do {\
    pid_t pid;\
    if ((pid=fork()) == 0) {\
        execvp(file, args);\
        perror("execvp");\
        exit(EXIT_FAILURE);\
    } else if (pid < 0) {\
        perror("fork");\
    } else {\
        int ret = waitpid(pid, NULL, 0);\
        if (ret == -1) {\
            perror("waitpid");\
            exit(EXIT_FAILURE);\
        }\
    }} while (0)


#define SAFE_MKDIR(path) do {\
    struct stat s;\
    const int stat_ret = stat(path, &s);\
    if (stat_ret == -1) {\
        if (errno == ENOENT) {\
            if (mkdir(path, S_IRWXU) != 0) {\
                perror("mkdir");\
            }\
        } else {\
            perror("stat");\
            exit(EXIT_FAILURE);\
        }\
    } else {\
        if (!S_ISDIR(s.st_mode)) {\
            fprintf(stderr, "%s already exists, but is not a directory!", g_cache_dir);\
            exit(EXIT_FAILURE);\
        }\
    }\
while (0)


#define CMD_CHECK_EXISTS(cmd) system("which " cmd " > /dev/null 2>&1")
#define CMD_READ_GIT_LSFILES(cmd) system(cmd " $(git ls-files)")


inline void append_err(dyn_arr *p_errors, const int pkg_name_len, const char* pkg_name, const char *err_msg) {
    char *pkg_name_cpy = malloc(pkg_name_len + 1);
    memcpy(pkg_name_cpy, pkg_name, pkg_name_len + 1);

    const size_t err_msg_len = strlen(err_msg);
    char *err_msg_cpy = malloc(err_msg_len + 1);
    memcpy(err_msg_cpy, err_msg, err_msg_len + 1);

    error_t err = {
        .pkg_name = pkg_name_cpy,
        .err_msg = err_msg_cpy,
    };
    dyn_arr_append(p_errors, 1, &err);
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

    int ret = sync_pkgs(sync_pkg_count, sync_pkg_list, makepkg_opts_len, makepkg_opts);
    for (int i = 0; i < makepkg_opts_len; i++) {
        free(makepkg_opts[i]);
    }
    return ret;
}

// TODO:
//  - tell when something (PKGBUILD changes, etc) is changed from the aur
//
// TODO: get deps, make deps, handle aur deps
//
// FILE *pipe = popen("source PKGBUILD; echo ${makedepends[*]} ${depends[*]}");
// pclose(pipe);
//
// NOTE: not accounting for when directory is bad (empty, deleted something in it, etc)
//
int sync_pkgs(const int sync_pkg_count, const char **sync_pkg_list, const int makepkg_opts_len, char *const makepkg_opts[]) {
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
    const char *ext_clone_dir = "/clone/";
    const int ext_clone_dir_len = strlen(ext_clone_dir);
    const int ext_clone_dir_path_len = cache_dir_len + ext_clone_dir_len;
    char ext_clone_dir_path[ext_clone_dir_path_len + 1];
    memcpy(ext_clone_dir_path, g_cache_dir, cache_dir_len);
    memcpy(ext_clone_dir_path + cache_dir_len, ext_clone_dir, ext_clone_dir_len + 1);

    SAFE_MKDIR(ext_clone_dir_path);

    char *initial_cwd = getcwd(NULL, 0);

    dyn_arr errors = dyn_arr_init(0, 0, sizeof (error_t), NULL);

    dyn_arr fetched_pkgs = fetch_pkgs(&errors,
                                      sync_pkg_count, sync_pkg_list,
                                      ext_clone_dir_path_len, ext_clone_dir_path);

    for (size_t i = 0; i < fetched_pkgs.size; i++) {
        pkginfo_t pkginfo = ((pkginfo_t*)fetched_pkgs.data)[i]

        if (pkginfo.diff == NO_CHANGES) {
            continue;
        }

        chdir(pkginfo.pkg_dir_path);
        if (pkginfo.diff == UPDATE) {
            system("git diff HEAD FETCH_HEAD");
        } else {
            if (CMD_CHECK_EXISTS("bat")) {
                CMD_READ_GIT_LSFILES("bat");
            } else if (CMD_CHECK_EXISTS("more")) {
                CMD_READ_GIT_LSFILES("more");
            } else if (CMD_CHECK_EXISTS("less")) {
                CMD_READ_GIT_LSFILES("less");
            } else if (CMD_CHECK_EXISTS("cat")) {
                system("cat PKGBUILD"); // NOTE: don't wanna risk reading a binary file with it
            } else {
                fprintf(stderr, BOLD_RED "Couldn't find suitable pager for reading" RCN);
            }
        }

        char str[8];
        bool accept = false;
        printf("Accept [Y/n]: ");
        if (fgets(str, sizeof (str), stdin)) {
            if (strcmp(str, "\n") == 0 ||
                strcmp(str, "y\n") == 0 ||
                strcmp(str, "Y\n") == 0
            ) {
                accept = true;
            } else { // flush input buf ig
                scanf("%*[^\n]");
                scanf("%*c");
            }
        } else {
            fprintf(stderr, BOLD_RED "Couldn't read input." RCN);
            exit(69);
        }

        if (!accept) {
            return 0;
        }
    }

    // git reset origin --hard
    dyn_arr built_pkgs = build_pkgs(&errors, fetched_pkgs);
    dyn_arr_free(&fetched_pkgs);

    install_pkgs(&errors, built_pkgs);
    dyn_arr_free(&built_pkgs);

    chdir(initial_cwd);
    free(initial_cwd);

    if (errors.size > 0) {
        printf(BOLD_RED "Errors found!" RCN);
        for (size_t i = 0; i < errors.size; i++) {
            error_t err = ((error_t*)errors.data)[i];
            char *pkg_name = err.pkg_name, *err_msg = err.err_msg;
            printf(BOLD_RED "pkg:" RC " %s\n", pkg_name);
            printf(BOLD_RED "reason:" RC " %s\n", err_msg);
            free(pkg_name);
            free(err_msg);
        }
    }
    dyn_arr_free(&errors);

    return 0;
}

inline dyn_arr fetch_pkgs(dyn_arr *p_errors,
                       int sync_pkg_count, const char **sync_pkg_list,
                       int ext_clone_dir_path_len, const char *ext_clone_dir_path) {
    dyn_arr fetched_pkgs = dyn_arr_init(sync_pkg_count, 0, sizeof (pkginfo_t), NULL);
    for (int i = 0; i < sync_pkg_count; i++) {
        const char* pkg_name = sync_pkg_list[i];
        const int pkg_dir_path_len = ext_clone_dir_path_len + pkg_name_len;
        char pkg_dir_path[pkg_dir_path_len + 1];
        memcpy(pkg_dir_path, ext_clone_dir_path, ext_clone_dir_path_len);
        memcpy(pkg_dir_path + ext_clone_dir_path_len, pkg_name, pkg_name_len + 1);

        e_diff_type diff = NO_CHANGES;
        bool err = false;
        struct stat s;
        const int ret = stat(pkg_dir_path, &s);
        if (ret == -1) {
            if (errno == ENOENT) {
                chdir(ext_clone_dir_path);
                printf(BOLD_GREEN "Cloning..." RCN);
                diff = NEW;

                const char *cmd = "git clone ";
                const int cmd_len = strlen(cmd);

                const char* ext_aur_pkg_url = EXT_AUR_PKG_URL;
                const int ext_aur_pkg_url_len = strlen(ext_aur_pkg_url);

                const char *suffix = ".git";
                const int suffix_len = strlen(suffix);

                char fullcmd[cmd_len + ext_aur_url_len + pkg_name_len + suffix_len + 1];
                memcpy(fullcmd,
                       cmd, cmd_len);
                memcpy(fullcmd + cmd_len,
                       ext_aur_pkg_url, ext_aur_pkg_url_len);
                memcpy(fullcmd + cmd_len + ext_aur_pkg_url_len,
                       pkg_name, pkg_name_len);
                memcpy(fullcmd + cmd_len + ext_aur_pkg_url_len + pkg_name_len,
                       suffix, suffix_len + 1);

                int ret = system(fullcmd);
                if (ret != 0) {
                    printf(BOLD_RED "An error occurred while cloning %s repo!" RCN, pkg_name);
                    err = true;
                    append_err(p_errors, pkg_name_len, pkg_name, "Git clone error");
                }
            } else {
                perror("stat");

                err = true;
                append_err(p_errors, pkg_name_len, pkg_name, "stat() error");
            }
        } else {
            if (!S_ISDIR(s.st_mode)) {
                fprintf(stderr, "%s already exists, but is not a directory!", pkg_dir_path);

                err = true;
                append_err(p_errors, pkg_name_len, pkg_name, "Package path is not a directory");
            } else {
                chdir(pkg_dir_path);

                printf(BOLD_GREEN "Fetching..." RCN);
                int ret = system("git fetch");
                if (ret != 0) {
                    printf(BOLD_RED "An error occurred while fetching repo!" RCN);

                    err = true;
                    append_err(p_errors, pkg_name_len, pkg_name, "Git fetch error");
                }
                FILE *p = popen("git diff HEAD FETCH_HEAD");
                char buf[1024];
                while (fread(buf, sizeof char, (sizeof buf) - 1, p) != NULL) {
                    buf[(sizeof buf) - 1] = '\0';
                    if (strlen(buf) != 0) {
                        diff = UPDATE;
                    }
                }
                pclose(p);
            }
        }

        if (!err) {
            char *pkg_name_cpy = malloc(pkg_name_len + 1);
            strcpy(pkg_name_cpy, pkg_name);

            char *pkg_dir_path_cpy = malloc(pkg_dir_path_len + 1);
            strcpy(pkg_dir_path_cpy, pkg_dir_path);

            pkginfo_t pkginfo = {
                .pkg_name = pkg_name_cpy,
                .pkg_dir_path = pkg_dir_path_cpy,
                .built_pkg_path = NULL,
                .diff = diff,
            };
            dyn_arr_append(&fetched_pkgs, 1, &pkginfo);
        }
    }
    return fetched_pkgs;
}

inline dyn_arr build_pkgs(dyn_arr *p_errors, dyn_arr fetched_pkgs) {
    dyn_arr built_pkgs = dyn_arr_init(fetched_pkgs.size, 0, sizeof pkginfo_t, NULL);
    for (size_t i = 0; i < fetched_pkgs.size; i++) {
        pkginfo_t pkginfo = ((pkginfo_t*)fetched_pkgs.data)[i];
        chdir(pkginfo.pkg_dir_path);
        system("git reset --hard origin");

        char *makepkg_args[1 + makepkg_opts_len + 1];
        makepkg_args[0] = "makepkg";
        memcpy(makepkg_args + 1, makepkg_opts, makepkg_opts_len * sizeof (char*));
        makepkg_args[makepkg_opts_len + 1] = NULL;

        printf(BOLD_GREEN "Running makepkg..." RCN);
        EXECVP("makepkg", makepkg_args);

        FILE *pipe = popen("makepkg --packagelist", "r");

        dyn_arr dyn_buf = dyn_arr_init(1, 0, sizeof (string), NULL);
        const size_t buf_size = 128;
        char buf[buf_size];
        int whole_buf_strlen = 0;
        while (fread(buf, sizeof char, buf_size - 1, pipe) != NULL) {
            buf[buf_size - 1] = '\0';
            size_t str_len = strlen(buf);
            whole_buf_strlen += str_len;

            char *str_data = malloc(str_len + 1);
            strcpy(str_data, buf);
            string str = {
                .size = str_len,
                .data = str_data,
            };

            dyn_arr_append(&dyn_buf, 1, &str);
        }

        pclose(pipe);

        char whole_buf[whole_buf_strlen + 1];
        int curr_size = 0;
        for (size_t i = 0; i < dyn_buf.size; i++) {
            string *data = (string*)dyn_buf.data;
            string str = data[i];

            memcpy(whole_buf + curr_size, str.data, str.size);
            curr_size += str.size;

            free(str.data);
        }
        whole_buf[whole_buf_strlen] = '\0'; // technically unnecessary
        dyn_arr_free(&dyn_buf);

        dyn_arr built_pkgs = dyn_arr_init(1, 0, sizeof (char*), NULL);
        for (int i = 0; i < whole_buf_strlen; i++) {
            int char_read = 0;
            int str_len = 0;
            for (int j = i; j < whole_buf_strlen; j++) {
                if (whole_buf[j] == '\n') {
                    break;
                }
                char_read++;
                str_len++;
            }

            char *pkg = malloc(str_len + 1);
            memcpy(pkg, whole_buf + i, str_len);
            pkg[str_len] = '\0';

            dyn_arr_append(&built_pkgs, 1, &pkg);

            i += char_read;
        }
    }
}

inline void install_pkgs() {
    printf(BOLD_GREEN "Installing..." RCN);

    char *sudo_args[4 + built_pkgs.size + 1];
    sudo_args[0] = "sudo";
    sudo_args[1] = "pacman";
    sudo_args[2] = "-U";
    sudo_args[3] = "--needed";
    for (size_t i = 0; i < built_pkgs.size; i++) {
        char *built_pkg = ((char**)built_pkgs.data)[i];
        sudo_args[4 + i] = built_pkg;
    }
    sudo_args[4 + built_pkgs.size] = NULL;

    pid_t pid;
    if ((pid=fork()) == 0) {
        execvp("sudo", sudo_args);
        perror("execvp");
        exit(1);
    } else if (pid < 0) {
        perror("fork");
    } else {
        int status;
        int ret = waitpid(pid, &status, 0);
        if (ret == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                printf(BOLD_RED "An error occurred while installing package!" RCN);
                printf(BOLD_RED "Skipping %s" RCN, pkg_name);

                char *pkg_name_cpy = malloc(pkg_name_len + 1);
                memcpy(pkg_name_cpy, pkg_name, pkg_name_len + 1);

                const char *err_msg = "Installation error";
                const size_t err_msg_len = strlen(err_msg);
                char *err_msg_cpy = malloc(err_msg_len + 1);
                memcpy(err_msg_cpy, err_msg, err_msg_len + 1);

                error_t err = {
                    .pkg_name = pkg_name_cpy,
                    .err_msg = err_msg_cpy,
                };
                dyn_arr_append(p_errors, 1, &err);
            }
        }
    }

    for (size_t i = 0; i < built_pkgs.size; i++) {
        char *built_pkg = ((char**)built_pkgs.data)[i];
        free(built_pkg);
    }
    dyn_arr_free(&built_pkgs);
}
