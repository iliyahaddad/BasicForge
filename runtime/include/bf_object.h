#ifndef BF_OBJECT_H
#define BF_OBJECT_H

#include <stdint.h>
#include <stddef.h>

// Object allocation
void* bf_object_alloc(size_t size);
void bf_object_free(void* obj);

#endif // BF_OBJECT_H