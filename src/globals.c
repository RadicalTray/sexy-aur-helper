#include "globals.h"
#include "types.h"

const Cmd g_cmds[g_cmds_len] = {
    { NO_CMD, "" },
    { SEARCH, "search" },
    { SYNC, "sync" },
    { UPGRADE, "upgrade" },
    { UPDATE_PKG_LIST, "update-pkg-list" },
    { CLEAR_CACHE, "clear-cache" },
};

const Opts g_default_opts = {
    .pkg_name = "",
    .makepkg_opts = "-si",
};

char *g_cache_dir;
char *g_pkg_list_filepath;
