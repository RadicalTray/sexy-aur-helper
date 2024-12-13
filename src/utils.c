#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// TODO: accept cd path, NULL = don't cd
int exec_sh_cmd(const char *cmd) {
    printf("Executing '%s'\n", cmd);
    return system(cmd);
}

void print_help(FILE *fptr) {
    fprintf(fptr, "HELP\n");
}
