#include <string.h>
#include <stdlib.h>

char* str_concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *str = malloc(len1 + len2 + 1);
    memcpy(str, s1, len1);
    memcpy(str + len1, s2, len2 + 1);
    return str;
}
