#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "globals.h"
#include "types.h"
#include "utils.h"
#include "sync.h"
#include "search.h"
#include "upgrade.h"
#include "cache.h"
#include "update.h"

void cleanup();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void SIGINT_HANDLER(int sig) {
    fprintf(stderr, BOLD_RED "Caught SIGINT! Exiting..." RCN);
    while(wait(NULL) > 0);
    exit(2);
}
#pragma GCC diagnostic pop

void setup() {
    if (signal(SIGINT, SIGINT_HANDLER) == SIG_ERR) {
        perror("SIGINT");
        exit(EXIT_FAILURE);
    }

    if (set_globals() != 0) {
        exit(EXIT_FAILURE);
    }

    struct stat s;
    const int stat_ret = stat(g_cache_dir, &s);
    if (stat_ret == -1) {
        if (errno == ENOENT) {
            if (mkdir(g_cache_dir, S_IRWXU) != 0) {
                perror("mkdir");
                exit(EXIT_FAILURE);
            }
            chdir(g_cache_dir);

            if (system("git init") != 0) {
                exit(EXIT_FAILURE);
            }
            if (system("git config --local init.defaultbranch master") != 0) {
                exit(EXIT_FAILURE);
            }
        } else {
            perror("stat");
            exit(EXIT_FAILURE);
        }
    } else {
        if (!S_ISDIR(s.st_mode)) {
            fprintf(stderr, "%s already exists, but is not a directory!", g_cache_dir);
            exit(EXIT_FAILURE);
        }
    }
}

int main(const int argc, const char **argv) {
    setup();

    if (argc <= 1) {
        print_help(stderr);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help(stdout);
        return 0;
    }

    CmdType cmdType = NO_CMD;
    for (int i = 0; i < g_cmds_len; i++) {
        if (strcmp(argv[1], g_cmds[i].cmdStr) == 0) {
            cmdType = g_cmds[i].cmdType;
            break;
        }
    }
    if (cmdType == NO_CMD) {
        fprintf(stderr, "%s is unexpected!\n", argv[1]);
        print_help(stderr);
        return 1;
    }

    const int cmd_args_len = argc - 2;
    const char **cmd_args = argv + 2;

    int ret;
    switch (cmdType) {
        case SEARCH:
            ret = run_search(cmd_args_len, cmd_args);
            break;
        case SYNC:
            ret = run_sync(cmd_args_len, cmd_args);
            break;
        case UPGRADE:
            ret = run_upgrade(cmd_args_len, cmd_args);
            break;
        case UPDATE_PKG_LIST:
            ret = run_update_pkg_list(cmd_args_len, cmd_args);
            break;
        case CLEAR_CACHE:
            ret = run_clear_cache(cmd_args_len, cmd_args);
            break;
        default:
            fprintf(stderr, "%s: This is impossible.\n", __func__);
            ret = 69;
            break;
    }

    cleanup();
    return ret;
}

// this is definitely overkill lol
void cleanup() {
    free_aur_pkg_list(&g_pkgbase_list);
    free(g_pkgbase_list_filepath);
    free_aur_pkg_list(&g_pkg_list);
    free(g_pkg_list_filepath);
    alpm_release(g_alpm_handle);
    free(g_cache_dir);
}
