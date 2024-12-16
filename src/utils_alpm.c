#include "utils_alpm.h"
#include "globals.h"
#include <alpm.h>

// should only be called once in a program
int init_alpm() {
    if (g_alpm_handle != NULL || g_alpm_localdb != NULL) {
        fprintf(stderr, "init_alpm() is called more than once\n");
        return 69;
    }

    alpm_errno_t err;
    g_alpm_handle = alpm_initialize("/", "/var/lib/pacman/", &err);
    if (g_alpm_handle == NULL) {
        fprintf(stderr, "alpm_initialize: %s\n", alpm_strerror(err));
        return 1;
    }

    g_alpm_localdb = alpm_get_localdb(g_alpm_handle);
    if (g_alpm_localdb == NULL) {
        fprintf(stderr, "Couldn't get localdb!\n");
        return 1;
    }
    return 0;
}

// I think this list or at least the data is freed along with db and alpm
//
// this function is called only once in a program i think.
// so no need to make a g_pkg_list
const alpm_list_t* get_pkg_list() {
    if (init_alpm() != 0) {
        return NULL;
    }

    alpm_list_t *alpm_pkg_list = alpm_db_get_pkgcache(g_alpm_localdb);
    if (alpm_pkg_list == NULL) {
        fprintf(stderr, "Couldn't get package list from database!");
        return NULL;
    }

    return alpm_pkg_list;
}

