#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "globals.h"
#include "types.h"
#include "functions.h"

int main(const int argc, const char **argv) {
    if (set_globals() != 0) {
        return 1;
    }

    mkdir(g_cache_dir, S_IRWXU);
    if (errno != -1 && errno != EEXIST) {
        perror("Error");
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
