#pragma once
#include <stdbool.h>

typedef enum {
    NO_CMD,
    SEARCH,
    SYNC,
    UPGRADE,
    UPDATE_PKG_LIST,
    CLEAR_CACHE,
} CmdType;

typedef struct {
    CmdType cmdType;
    char* cmdStr;
} Cmd;

typedef struct {
    char *pkg_name;
    char *makepkg_opts;
} Opts;

typedef struct pkg_list_s {
    int size;
    char *buf;
} pkg_list_t;
