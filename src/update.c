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
    const char *sh_cmd = "curl https://aur.archlinux.org/packages.gz | gzip -cd > ";
    const int sh_cmd_len = strlen(sh_cmd);
    const int filepath_len = strlen(g_pkg_list_filepath);
    const int cmd_len = sh_cmd_len + filepath_len;
    char cmd[cmd_len + 1]; // This should allocate it on the stack right?
    memcpy(cmd, sh_cmd, sh_cmd_len);
    memcpy(cmd + sh_cmd_len, g_pkg_list_filepath, filepath_len + 1);

    // TODO: use exec()
    if (exec_sh_cmd(cmd) != 0) {
        fprintf(stderr, "An error happended while trying to fetch the package list!\n");
        return 1;
    }
    return 0;
}

// returns file contents and filesize of the package.txt
//
// don't modify aur_pkg_list_t
const aur_pkg_list_t* get_aur_pkg_list() {
    if (!g_search_list.init) {
        g_search_list.init = true;

        if (access(g_pkg_list_filepath, F_OK) != 0) {
            if (run_update_pkg_list(0, NULL) != 0) {
                fprintf(stderr, "Couldn't fetch the package list!\n");
                return NULL;
            }
        }

        FILE *p_file = fopen(g_pkg_list_filepath, "r");
        if (p_file == NULL) {
            fprintf(stderr, PKG_LIST_FILENAME " couldn't be opened!\n");
            return NULL;
        }

        struct stat pkg_list_filestat;
        stat(g_pkg_list_filepath, &pkg_list_filestat);

        const int filesize = pkg_list_filestat.st_size;
        char *pkg_list_buf = malloc(filesize);
        fread(pkg_list_buf, filesize, 1, p_file); // last char of pkg_list_buf should be char '\n' or int = 10
        fclose(p_file);

        g_search_list.size = filesize;
        g_search_list.buf = pkg_list_buf;
    }

    return &g_search_list;
}
