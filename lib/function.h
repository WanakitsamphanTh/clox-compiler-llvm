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
    FN
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

typedef struct _CallFrame {
    ObjCallable* fn;
    uint8_t* ip;
    Value* slots;
    Chunk* chunk;
    Obj* error;
} CallFrame;

ObjCallable* newFunction(ObjString* name, int arity);
ObjCallable* newNativeFunction(ObjString* name, int arity, NativeFn callee);

#define IS_FUNCTION(val) isObjType(val, OBJ_FN)
#define AS_FUNCTION(val) ((ObjCallable*)AS_OBJ(val))

bool call(ObjCallable*, VM*);

#endif