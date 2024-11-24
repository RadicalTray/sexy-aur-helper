#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "globals.h"
#include "types.h"
#include "functions.h"

#include <alpm.h>

int cmp_find_aur(const void *a, const void *b);

int test(const int argc, const char **argv) {
    alpm_errno_t err;
    alpm_handle_t *g_alpm_handle = alpm_initialize("/", "/var/lib/pacman/", &err);
    if (g_alpm_handle == NULL) {
        fprintf(stderr, "alpm_initialize: %s", alpm_strerror(err));
        return 1;
    }

    alpm_db_t *db = alpm_get_localdb(g_alpm_handle);
    if (db == NULL) {
        fprintf(stderr, "Couldn't get localdb!");
        return 1;
    }
    // alpm_db_set_usage(db, ALPM_DB_USAGE_SEARCH);
    alpm_list_t *pkg_list = alpm_db_get_pkgcache(db);
    if (pkg_list == NULL) {
        fprintf(stderr, "Couldn't get package list from database!");
        return 1;
    }

    return alpm_release(g_alpm_handle); // this also releases db
}

int main(const int argc, const char **argv) {
    return test(argc, argv);

    if (set_globals() != 0) {
        return 1;
    }

    mkdir(g_cache_dir, S_IRWXU);
    if (errno != -1 && errno != EEXIST) {
        perror("mkdir");
    }

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
