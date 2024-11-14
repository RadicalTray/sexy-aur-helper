#pragma once
#include "types.h"

Opts process_args(const int len, char **args);
int run_search(const int len, const char **args);
int run_sync(const int len, const char **args);
int run_upgrade(const int len, const char **args);
int run_update_pkg_list(const int len, const char **args);
int run_clear_cache(const int len, const char **args);
void process_opt();
void process_long_opt();
void print_help();
