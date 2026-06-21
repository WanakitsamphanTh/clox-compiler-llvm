#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef double Value;

typedef struct {
    int count;
    int capacity;
    Value* values;
} ValueArray;

ValueArray newValueArray();
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#endif