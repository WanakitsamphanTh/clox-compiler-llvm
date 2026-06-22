#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef enum {
    NUMBER_VALUE,
    STR_VALUE,
    BOOL_VALUE,
    NIL_VALUE
} ValueType;

typedef struct {
    ValueType type;
    union {
        double num;
        bool b;
        const char* str;
    } val;
} Value;

typedef struct {
    int count;
    int capacity;
    Value* values;
} ValueArray;

ValueArray newValueArray();
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#define VALUE_BOOL(value) ((Value){.type = BOOL_VALUE, .val.b = value})
#define VALUE_NUM(value) ((Value){.type = NUMBER_VALUE, .val.num = value})
#define VALUE_NIL ((Value){.type = NIL_VALUE })

#define AS_BOOL(value) ((value).val.b)
#define AS_NUM(value) ((value).val.num)

void valueToString(Value, char*);

#endif