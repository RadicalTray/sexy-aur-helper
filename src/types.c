#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"

dyn_arr dyn_arr_init(const int cap, const int size, const void *data) {
    dyn_arr arr;

    if (data == NULL || size <= 0) {
        if (cap < 0) {
            arr.cap = 1;
            arr.buf = malloc(arr.cap);
        } else if (cap == 0) {
            arr.cap = 0;
            arr.buf = NULL;
        } else {
            arr.cap = cap;
            arr.buf = malloc(arr.cap);
        }

        arr.size = 0;
        return arr;
    }

    if (cap < size && cap >= 0) {
        fprintf(stderr, "CAP < SIZE, YOU IDIOT");
        exit(420);
    }

    if (cap < 0) {
        int new_cap = 1;
        while (new_cap < size) {
            new_cap = new_cap << 1;
        }
        arr.cap = new_cap;
    } else {
        arr.cap = cap;
    }

    arr.buf = malloc(arr.cap);
    memcpy(arr.buf, data, size);
    arr.size = size;

    return arr;
}

void dyn_arr_append(dyn_arr *arr, const int size, const void *data) {
    const int new_size = arr->size + size;
    if (new_size > arr->cap) {
        int new_cap = 1;
        while (new_cap < new_size) {
            new_cap = new_cap << 1;
        }
        arr->cap = new_cap;

        void *new_buf = malloc(arr->cap);
        memcpy(new_buf, arr->buf, arr->size);
        free(arr->buf);
        arr->buf = new_buf;
    }

    memcpy(arr->buf + arr->size, data, size);
    arr->size = new_size;
}

void dyn_arr_free(dyn_arr *arr) {
    free(arr->buf);
    arr->cap = 0;
    arr->size = 0;
    arr->buf = NULL;
}

void dyn_arr_resize(dyn_arr *arr, const int size);

void dyn_arr_reserve(dyn_arr *arr, const int size);

void print_dyn_arr_info(const dyn_arr *arr) {
    printf("dyn_arr:\n");
    printf("\tcap: %i\n", arr->cap);
    printf("\tsize: %i\n", arr->size);
    printf("\tbuf: %p\n", arr->buf);
}
