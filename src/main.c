#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "types.h"
#include "functions.h"

int main(int argc, char **argv) {
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
    char **cmd_args = argv + 2;
    const Opts opts = process_args(cmd_args_len, cmd_args);
    return run(cmdType, opts);
}
