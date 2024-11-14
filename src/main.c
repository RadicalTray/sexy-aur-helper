#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "types.h"
#include "functions.h"

int main(const int argc, const char **argv) {
    if (argc == 1) {
        print_help();
        return 1;
    }

    CmdType cmdType = NO_CMD;
    for (int i = 0; i < g_cmds_len; i++) {
        if (strcmp(argv[1], g_cmds[i].cmdStr) == 0) {
            cmdType = g_cmds[i].cmdType;
            break;
        }
    }
    if (cmdType == NO_CMD) {
        printf("%s is unexpected!\n", argv[1]);
        print_help();
        return 1;
    }

    const int cmd_args_len = argc - 2;
    const char **cmd_args = argv + 2;

    switch (cmdType) {
        case SEARCH:
            return run_search(cmd_args_len, cmd_args);
        case SYNC:
            return run_sync(cmd_args_len, cmd_args);
        case UPGRADE:
            return run_upgrade(cmd_args_len, cmd_args);
        case UPDATE_PKG_LIST:
            return run_update_pkg_list(cmd_args_len, cmd_args);
        case CLEAR_CACHE:
            return run_clear_cache(cmd_args_len, cmd_args);
        default:
            printf("WTF\n");
            return 69;
    }
}
