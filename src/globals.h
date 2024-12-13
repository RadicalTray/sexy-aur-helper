#pragma once
#include "types.h"
#include <alpm.h>

#define PKG_LIST_FILENAME "packages.txt"
#define XDG_CACHE_HOME "XDG_CACHE_HOME"
#define PROGRAM_NAME "saur"
#define EXT_AUR_PKG_URL "https://aur.archlinux.org/"

#define g_cmds_len 6
extern const Cmd g_cmds[g_cmds_len];

extern const Opts g_default_opts;
extern char *g_cache_dir;
extern char *g_pkg_list_filepath;
extern alpm_handle_t *g_alpm_handle;
extern alpm_db_t *g_alpm_localdb;
