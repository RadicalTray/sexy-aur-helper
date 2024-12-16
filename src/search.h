int run_search(const int len, const char **args);
void search_pkg(const int search_strslen,
                const char **search_strs,
                const int pkg_list_size,
                const char *pkg_list,
                int *dst_matched_pkgs_count,
                char ***dst_matched_pkgs);
int pkg_is_in_aur(const int sync_pkg_name_len, const char *sync_pkg_name);
