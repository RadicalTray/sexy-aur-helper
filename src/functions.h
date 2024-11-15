#pragma once
#include "types.h"

int set_globals();
int run_search(const int len, const char **args);
int run_sync(const int len, const char **args);
int run_upgrade(const int len, const char **args);
int run_update_pkg_list(const int len, const char **args);
int run_clear_cache(const int len, const char **args);
void cleanup();
void print_help(FILE *fptr);
