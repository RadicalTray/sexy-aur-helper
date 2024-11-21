#include "types.h"

dyn_arr dyn_arr_init(const int cap, const int size, const void *data) {
    if (cap < 0) {
        return {}
    }
    void *data_cpy = malloc(cap);
    if () {
    }
    memcpy(data_cpy, data, size);
    return (dyn_arr){ .cap = cap, .size = size, .data = data_cpy };
}

void dyn_arr_append(dyn_arr *arr, const int size, const void *data) {
    if (arr.size + size > arr.cap) {
        void *new_buf = malloc();
    }

    memcpy(arr.buf + arr.size, data, size);
}

void dyn_arr_resize(dyn_arr *arr, const int size) {}

void dyn_arr_reserve(dyn_arr *arr, const int size) {}
