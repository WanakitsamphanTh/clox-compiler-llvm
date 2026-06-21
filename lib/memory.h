#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include "common.h"

int growCapacity(int capacity);
void* growArray(size_t size, void* array, int old_capacity, int capacity);
void* reallocate(void* ptr, size_t old_size, size_t new_size);
void* freeArray(size_t size, void* array, int capacity);

#endif