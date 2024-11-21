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

typedef struct dyn_ahh_arr {
    int cap;
    int size;
    void *buf;
} dyn_arr;

dyn_arr dyn_arr_init(const int cap, const int size, const void *data);
void dyn_arr_append(dyn_arr *arr, const int size, const void *data);
void dyn_arr_resize(dyn_arr *arr, const int size);
void dyn_arr_reserve(dyn_arr *arr, const int size);
