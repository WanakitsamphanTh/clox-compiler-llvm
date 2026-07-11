#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef enum {
    OBJ_INSTANCE,
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_CALLABLE
} ObjType;

typedef struct _Obj {
    // meta info
    void (*destructor)(void*);
    ObjType type;

    // next
    struct _Obj* next;
} Obj;

Obj* AllocateObj(ObjType type, void (*destructor)(void*), size_t size);

typedef enum {
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

void freeObj(Obj*);
bool isObjType(Value, ObjType);
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
#define AS_STR(value) ((ObjString*) (value).val.obj)
#define AS_ARRAY(value) ((ObjArray*) (value).val.obj)
#define AS_CSTR(value) AS_STR(value)->str

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STR(value) isObjType(value, OBJ_STRING)
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_CALLABLE(value) isObjType(value, OBJ_CALLABLE)

#define IS_TRUTHY(value) (((value).type == BOOL_VALUE) && ((value).val.b == true))
#define IS_FALSY(value) !IS_TRUTHY(value)

typedef struct {    
    Obj obj;
    size_t len;     /*len = strlen(str) (not including '\0')*/
    uint32_t hash;
    char str[];
} ObjString;

uint32_t hashString(const char*, int);
ObjString* makeObjString(const char*, int);
ObjString* newObjString(const char*, size_t, uint32_t);
ObjString* concatObjString(const ObjString*, const ObjString*);
ObjString* valueToObjString(Value);

typedef struct {
    Obj obj;
    size_t len;
    Value elements[];
} ObjArray;

ObjArray* makeObjArray(size_t, Value*);
ObjArray* concatObjArray(const ObjArray*, const ObjArray*);

#endif