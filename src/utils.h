#include "types.h"
#include <stdio.h>

void print_help(FILE *fptr);
int exec_sh_cmd(const char *cmd);
int set_globals();
char* str_concat(const char *s1, const char *s2);
