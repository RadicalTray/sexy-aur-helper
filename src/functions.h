#pragma once
#include "types.h"

Opts process_args(const int len, char **args);
int run(CmdType cmdType, Opts opts);
void process_opt();
void process_long_opt();
void print_help();
