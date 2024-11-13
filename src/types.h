#pragma once
#include <stdbool.h>

enum ECmdType_s {
    NO_CMD,
    SEARCH,
    SYNC,
    UPGRADE,
    UPDATE_PKG_LIST,
    CLEAR_CACHE,
};
typedef enum ECmdType_s CmdType;

struct Cmd_s {
    CmdType cmdType;
    char* cmdStr;
};
typedef struct Cmd_s Cmd;

struct Opts_s {
    char *pkg_name;
    char *makepkg_opts;
};
typedef struct Opts_s Opts;
