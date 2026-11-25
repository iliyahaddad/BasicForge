#include "bf_array.h"
#include <stdlib.h>

int64_t bf_array_len(void* arr) {
    if (!arr) return 0;
    return *(int64_t*)arr;
}

void* bf_array_alloc(int64_t size, int64_t elem_size) {
    // Allocate size+1 elements: first element is the length
    void* arr = malloc((size + 1) * elem_size);
    if (arr) {
        *(int64_t*)arr = size;
    }
    return arr;
}

void* bf_array_alloc_1d(int64_t size, int64_t elem_size) {
    return bf_array_alloc(size, elem_size);
}