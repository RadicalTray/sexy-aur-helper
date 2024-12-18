#include "globals.h"
#include "types.h"
#include <alpm.h>

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

char *g_cache_dir = NULL;
char *g_pkg_list_filepath = NULL;
char *g_pkgbase_list_filepath = NULL;
aur_pkg_list_t g_search_list = { .init = false, .size = 0, .buf = NULL };
aur_pkg_list_t g_pkgbase_list = { .init = false, .size = 0, .buf = NULL };
alpm_handle_t *g_alpm_handle = NULL;
alpm_db_t *g_alpm_localdb = NULL;
