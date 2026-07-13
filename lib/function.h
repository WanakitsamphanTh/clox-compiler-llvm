#ifndef FUNCTION_H
#define FUNCTION_H
#include "value.h"
#include "chunk.h"
#include "vm.h"

typedef struct _Value Value;
typedef struct _Obj Obj;
typedef struct _FnDeclStmt FnDeclStmt;
typedef struct _CallFrame CallFrame;

//typedef struct _VM VM;
extern VM vm;
extern Chunk newChunk();

typedef enum {
    NAT_FN,
    FN,
    CLOSURE
} FunctionType;

typedef struct _ObjCallable ObjCallable;
typedef Value (*NativeFn)(VM*);

typedef struct _ObjCallable {
    Obj obj;
    int arity;
    FunctionType type;
    ObjString* name;
} ObjCallable;

typedef struct {
    ObjCallable invoke;
    NativeFn fn;
} ObjNativeFn;

typedef struct {
    ObjCallable invoke;
    Chunk chunk;
} ObjFn;

typedef struct _ObjUpValue {
    Obj obj;
    Value* ref;
    Value value;
} ObjUpValue;

typedef struct {
    ObjCallable invoke;
    ObjFn* prototype;
    size_t upvalue_count;
    ObjUpValue *upvalues[];
} ObjClosure;

typedef struct _CallFrame {
    ObjCallable* fn;
    uint8_t* ip;
    Value* slots;
    Chunk* chunk;
    Obj* error;
} CallFrame;

ObjCallable* newFunction(ObjHeap*, ObjString* name, int arity);
ObjCallable* newNativeFunction(ObjHeap*, ObjString* name, int arity, NativeFn callee);
ObjCallable* newClosure(ObjHeap*, ObjFn*, size_t);

ObjUpValue* newUpValue(ObjHeap*, Value*);
void closeUpValue(ObjUpValue*);
Value getUpValue(ObjUpValue*);
void setUpValue(ObjUpValue*, Value);

#define AS_CALLABLE(val) ((ObjCallable*)AS_OBJ(val))

bool call(ObjCallable*, VM*);

#endif