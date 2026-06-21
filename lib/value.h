#ifndef VALUE_H
#define VALUE_H

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