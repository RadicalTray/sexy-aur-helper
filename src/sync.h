int run_sync(const int len, const char **args);
int sync_pkg(const int sync_pkg_count, const char **sync_pkg_list, const int makepkg_opts_len, char *const makepkg_opts[]);
int run_makepkg(const int clone_dir_path_len,
                const char *clone_dir_path,
                const int makepkg_opts_len,
                char *const makepkg_opts[],
                const int sync_pkg_count,
                const char **sync_pkg_list);
