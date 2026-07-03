#include "memory.h"
#include <stdlib.h>

int growCapacity(int capacity){
    return (capacity) < 8 ? 8 : capacity * 2;
}

void* growArray(size_t size, void* array, int old_capacity, int capacity) {
    return reallocate(array, size * old_capacity, size * capacity);
}

void* freeArray(size_t size, void* array, int capacity){
    return reallocate(array, size * capacity, 0);
}

void* reallocate(void* ptr, size_t old_size, size_t new_size){
    if(new_size == 0) {
        free(ptr);
        return NULL;
    }

    void* result = realloc(ptr, new_size);
    return result;
}
