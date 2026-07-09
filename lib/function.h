#ifndef FUNCTION_H
#define FUNCTION_H
#include "value.h"
#include "chunk.h"

typedef struct _Value Value;
typedef struct _Obj Obj;
typedef struct _FnDeclStmt FnDeclStmt;

typedef struct {
    Obj* error;
} ErrorHandler;

typedef enum {
    NAT_FN,
    FN
} FunctionType;

typedef struct {
    Obj obj;
    int arity;
    ObjString* name;
    ErrorHandler handler;
    Value (*call)(ErrorHandler*, Value*);
} ObjCallable, ObjNativeFn;

typedef struct {
    ObjCallable callee;
    Chunk chunk;
} ObjFn;

typedef struct {
    ObjCallable fn;
    uint8_t* ip;
    Value* slot;
} CallFrame;

ObjCallable* newFunction(FunctionType type, int arity, ObjString* name);

#define IS_FUNCTION(val) isObjType(val, OBJ_FN)
#define AS_FUNCTION(val) ((ObjCallable*)AS_OBJ(val))

Value _nat_scan(ErrorHandler*, Value*);
Value _nat_scan_ln(ErrorHandler*, Value*);
Value _nat_scan_num(ErrorHandler*, Value*);
Value _nat_clock(ErrorHandler*, Value*);
Value _nat_map(ErrorHandler*, Value*);


#endif