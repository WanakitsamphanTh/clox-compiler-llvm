#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef struct _Obj Obj;

typedef enum _ValueType {
    NIL_VALUE = 0,

    // Value type
    NUMBER_VALUE,
    BOOL_VALUE,

    // Reference type
    OBJ_VALUE,
} ValueType;

typedef struct _Value {
    ValueType type;
    union {
        double num;
        bool b;
        Obj* obj;
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

//void valueToString(Value, char*, int, int);
int valueToString(Value v, char* buffer, int max_length);

void encodeString(char*, const char*);
char* decodeString(char*);

bool compareValue(Value, Value);

#define IS_BOOL(value) ((value).type == BOOL_VALUE)
#define IS_NUM(value) ((value).type == NUMBER_VALUE)
#define IS_NIL(value) ((value).type == NIL_VALUE)

#define VALUE_BOOL(value) ((Value){.type = BOOL_VALUE, .val.b = value})
#define VALUE_NUM(value) ((Value){.type = NUMBER_VALUE, .val.num = value})
#define VALUE_NIL ((Value){.type = NIL_VALUE })
#define VALUE_OBJ(ptr) ((Value){.type = OBJ_VALUE, .val.obj = ptr})

#define AS_BOOL(value) ((value).val.b)
#define AS_NUM(value) ((value).val.num)
#define AS_OBJ(value) ((value).val.obj)
#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STR(value) isObjType(value, OBJ_STRING)
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_CALLABLE(value) isObjType(value, OBJ_CALLABLE)

#define IS_TRUTHY(value) (((value).type == BOOL_VALUE) && ((value).val.b == true))
#define IS_FALSY(value) !IS_TRUTHY(value)

#endif