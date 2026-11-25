#include "bf_object.h"
#include <stdlib.h>

void* bf_object_alloc(size_t size) {
    return malloc(size);
}

void bf_object_free(void* obj) {
    if (obj) free(obj);
}