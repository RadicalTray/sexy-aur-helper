#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t size;
    char* data;
} string;

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
    char *cmdStr;
} Cmd;

typedef struct {
    char *pkg_name;
    char *makepkg_opts;
} Opts;

typedef struct _aur_pkg_list_t {
    bool init;
    int size;
    char *buf;
} aur_pkg_list_t;

typedef struct _dyn_arr {
    size_t cap;
    size_t size;
    size_t dtype_size;
    void *data;
} dyn_arr;

dyn_arr dyn_arr_init(const size_t cap, const size_t size, const size_t dtype_size, const void *data);

// NOTE: assumes data has the same type as darr
void dyn_arr_append(dyn_arr *darr, const int size, const void *data);

// not impl
void dyn_arr_resize(dyn_arr *darr, const int size);

// not impl
void dyn_arr_reserve(dyn_arr *darr, const int size);

void dyn_arr_free(dyn_arr *darr);

void free_aur_pkg_list(aur_pkg_list_t *li);
