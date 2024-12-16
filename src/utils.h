#include "types.h"
#include <stdio.h>

void print_help(FILE *fptr);
int exec_sh_cmd(const char *cmd);
const aur_pkg_list_t* get_aur_pkg_list();
int set_globals();
char* str_concat(const char *s1, const char *s2);
