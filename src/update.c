#include "update.h"
#include "globals.h"
#include "types.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

int run_update_pkg_list(const int len, const char **args) {
    if (len != 0) {
        fprintf(stderr, "Arguments unexpected: ");
        for (int i = 0; i < len - 1; i++) {
            fprintf(stderr, "'%s' ", args[i]);
        }
        fprintf(stderr, "'%s'\n", args[len - 1]);
        print_help(stderr);
        return 1;
    }

    // smartass
    char *sh_cmd = "curl https://aur.archlinux.org/packages.gz | gzip -cd > ";
    int sh_cmd_len = strlen(sh_cmd);
    int filepath_len = strlen(g_pkg_list_filepath);
    int cmd_len = sh_cmd_len + filepath_len;
    char cmd1[cmd_len + 1];
    memcpy(cmd1, sh_cmd, sh_cmd_len);
    memcpy(cmd1 + sh_cmd_len, g_pkg_list_filepath, filepath_len + 1);

    if (exec_sh_cmd(cmd1) != 0) {
        fprintf(stderr, "An error happended while trying to fetch the package list!\n");
        return 1;
    }

    sh_cmd = "curl https://aur.archlinux.org/pkgbase.gz | gzip -cd > ";
    sh_cmd_len = strlen(sh_cmd);
    filepath_len = strlen(g_pkgbase_list_filepath);
    cmd_len = sh_cmd_len + filepath_len;
    char cmd2[cmd_len + 1];
    memcpy(cmd2, sh_cmd, sh_cmd_len);
    memcpy(cmd2 + sh_cmd_len, g_pkgbase_list_filepath, filepath_len + 1);

    if (exec_sh_cmd(cmd2) != 0) {
        fprintf(stderr, "An error happended while trying to fetch the package list!\n");
        return 1;
    }
    return 0;
}

const aur_pkg_list_t* get(aur_pkg_list_t *li, const char* li_filepath) {
    if (!li->init) {
        li->init = true;

        if (access(g_pkg_list_filepath, F_OK) != 0) {
            if (run_update_pkg_list(0, NULL) != 0) {
                fprintf(stderr, "Couldn't fetch the package list!\n");
                return NULL;
            }
        }

        FILE *p_file = fopen(li_filepath, "r");
        if (p_file == NULL) {
            fprintf(stderr, PKG_LIST_FILENAME " couldn't be opened!\n");
            return NULL;
        }

        struct stat pkg_list_filestat;
        stat(li_filepath, &pkg_list_filestat);

        const int filesize = pkg_list_filestat.st_size;
        char *pkg_list_buf = malloc(filesize);
        fread(pkg_list_buf, filesize, 1, p_file); // last char of pkg_list_buf should be char '\n' or int = 10
        fclose(p_file);

        li->size = filesize;
        li->buf = pkg_list_buf;
    }

    return li;
}

// returns file contents and filesize of the package.txt
//
// don't modify aur_pkg_list_t
const aur_pkg_list_t* get_aur_pkg_list() {
    return get(&g_pkg_list, g_pkg_list_filepath);
}

const aur_pkg_list_t* get_aur_pkgbase_list() {
    return get(&g_pkgbase_list, g_pkgbase_list_filepath);
}
