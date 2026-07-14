#ifndef OBJ_H
#define OBJ_H
#include "common.h"
#include "value.h"

typedef struct _Value Value;
typedef struct _Table Table;

typedef enum _ObjType {
    OBJ_INSTANCE,
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_CALLABLE,
    OBJ_UPVALUE,
} ObjType;

typedef struct _Obj {
    // meta info
    struct _Obj_Vtable {
        void (*destructor)(Obj*);
        void (*mark)(Obj*);
    }* vtable;
    ObjType type;

    // for GC
    bool marked;

    // next
    struct _Obj* next;
} Obj;

void obj_mark(Obj*);
extern struct _Obj_Vtable obj_vtable;

typedef struct {
    Obj* objects;
    Table* strings;
} ObjHeap;

void initObjHeap(ObjHeap*);
void transferObjHeap(ObjHeap*,ObjHeap*);
void freeObjHeap(ObjHeap*);
void freeObj(Obj*);
bool isObjType(Value, ObjType);

Obj* AllocateObj(ObjHeap*, ObjType type, struct _Obj_Vtable*, size_t size);

typedef struct _ObjString {    
    Obj obj;
    size_t len;     /*len = strlen(str) (not including '\0')*/
    uint32_t hash;
    char str[];
} ObjString;

uint32_t hashString(const char*, int);
ObjString* makeObjString(ObjHeap*, const char*, int);
ObjString* newObjString(ObjHeap*, const char*, size_t, uint32_t);
ObjString* concatObjString(ObjHeap*, const ObjString*, const ObjString*);
ObjString* valueToObjString(ObjHeap*, Value);

typedef struct {
    Obj obj;
    size_t len;
    Value elements[];
} ObjArray;

ObjArray* makeObjArray(ObjHeap*, size_t, Value*);
ObjArray* concatObjArray(ObjHeap*, const ObjArray*, const ObjArray*);

#define AS_STR(value) ((ObjString*) (value).val.obj)
#define AS_ARRAY(value) ((ObjArray*) (value).val.obj)
#define AS_CSTR(value) AS_STR(value)->str

#endif