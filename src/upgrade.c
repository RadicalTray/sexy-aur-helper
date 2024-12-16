#include "upgrade.h"
#include "search.h"
#include "utils_alpm.h"
#include "sync.h"
#include "types.h"
#include "utils.h"
#include <alpm.h>
#include <string.h>

int upgrade_pkgs(int upgrade_pkg_count, const char *upgrade_pkg_list[]);

// TODO:
//  - parse args
//  - impl aur dep install if needed
int run_upgrade(const int len, const char **args) {
    if (len != 0) {
        printf("Individual pkg upgrade is not implemented.\n");
        print_help(stderr);
        return 1;
    }

    const alpm_list_t *alpm_pkg_list = get_pkg_list();
    dyn_arr pkg_names = dyn_arr_init(0, 0, NULL);
    for (const alpm_list_t *p = alpm_pkg_list; p != NULL; p = alpm_list_next(p)) {
        alpm_pkg_t *pkg = p->data;
        const char *packager = alpm_pkg_get_packager(pkg);

        // WARN: Assuming unknown packager = AUR
        if (strcmp(packager, "Unknown Packager") == 0) {
            // pretty sure i can modify it through dyn_arr
            const char *name = alpm_pkg_get_name(pkg);
            int result = pkg_is_in_aur(strlen(name), name);
            switch (result) {
                case 0: {
                    dyn_arr_append(&pkg_names, sizeof (char*), &name);
                    break;
                }
                case 1: {
                    fprintf(stdout, "'%s' not found in aur package list.\n", name);
                    break;
                } case 69: {
                    return 69;
                }
            }
        }
    }

    int ret = upgrade_pkgs(pkg_names.size / sizeof (char*), (const char**)pkg_names.buf);
    dyn_arr_free(&pkg_names);
    return ret;
}

int upgrade_pkgs(int upgrade_pkg_count, const char *upgrade_pkg_list[]) {
    // TODO: check if needed to upgrade, deps etc

    // BUG:
    upgrade_pkg_count = 2;
    upgrade_pkg_list = (const char*[]){ "clifm", "clifm-colors-git" };
    return sync_pkg(upgrade_pkg_count, upgrade_pkg_list, 0, NULL);
}
