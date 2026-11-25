#ifndef BF_ARRAY_H
#define BF_ARRAY_H

#include <stdint.h>

// Array operations
int64_t bf_array_len(void* arr);
void* bf_array_alloc(int64_t size, int64_t elem_size);
void* bf_array_alloc_1d(int64_t size, int64_t elem_size);

#endif // BF_ARRAY_H