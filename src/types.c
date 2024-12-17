#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

dyn_arr dyn_arr_init(const size_t cap, const size_t size, const size_t dtype_size, const void *data) {
    dyn_arr darr;

    if (cap < size) {
        darr.cap = darr.size;
    } else {
        darr.cap = cap;
    }

    darr.dtype_size = dtype_size;
    darr.data = malloc(darr.cap * darr.dtype_size);
    memcpy(darr.data, data, size * darr.dtype_size);
    darr.size = size;

    return darr;
}

void dyn_arr_append(dyn_arr *darr, const int size, const void *data) {
    const size_t new_size = darr->size + size;
    if (darr->cap < new_size) {
        // PERF: this might be wasteful prob could set it and forget it somewhere in code.
        //  but this shouldn't matter cuz cap < new_size should happen only a few times if
        //  u write code correctly
        //
        // count size_t bits
        // this is possible since size_t is unsigned
        // and behaviour of shifting bits out is defined
        int bit_count = 0;
        for (size_t i = 1; i != 0; i <<= 1) {
            bit_count++;
        }

        size_t new_cap = 0;
        for (int i = 0; new_cap < new_size && i < bit_count; i++) {
            size_t x = 1;
            for (int j = 0; new_cap + x < new_size && j < bit_count - j - 1; j++) {
                x <<= 1;
            }
            new_cap += x;
        }

        darr->cap = new_cap;

        void *new_buf = malloc(darr->cap * darr->dtype_size);
        memcpy(new_buf, darr->data, darr->size * darr->dtype_size);
        free(darr->data);
        darr->data = new_buf;
    }

    memcpy(darr->data + darr->size * darr->dtype_size,
           data,
           size * darr->dtype_size);
    darr->size = new_size;
}

inline void dyn_arr_free(dyn_arr *darr) {
    free(darr->data);
    darr->cap = 0;
    darr->size = 0;
    darr->data = NULL;
}

void dyn_arr_resize(dyn_arr *darr, const int size);

void dyn_arr_reserve(dyn_arr *darr, const int size);

void print_dyn_arr_info(const dyn_arr *darr) {
    printf("dyn_arr:\n");
    printf("\tcap: %li\n", darr->cap);
    printf("\tsize: %li\n", darr->size);
    printf("\tdtype_size: %li\n", darr->dtype_size);
    printf("\tbuf: %p\n", darr->data);
}

inline void free_aur_pkg_list(aur_pkg_list_t *li) {
    free(li->buf);
    li->init = false;
    li->size = 0;
    li->buf = NULL;
}
