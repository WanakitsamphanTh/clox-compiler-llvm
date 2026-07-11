#include "function.h"
#include "string.h"
#include "stdio.h"
#include "value.h"

void free_obj_fn(void* obj){
    ObjFn* fn = obj;
    freeChunk(&fn->chunk);
}

ObjCallable* newFunction(ObjString* name, int arity){
    ObjFn* fn = AllocateObj(OBJ_CALLABLE, NULL, sizeof(ObjFn));
    fn->invoke.name = name;
    fn->invoke.arity = arity;
    fn->invoke.type = FN;
    fn->chunk = newChunk();
    return fn;
}
ObjCallable* newNativeFunction(ObjString* name, int arity, NativeFn callee){
    ObjNativeFn* fn = AllocateObj(OBJ_CALLABLE, NULL, sizeof(ObjNativeFn));
    fn->fn = callee;
    fn->invoke.type = NAT_FN;
    fn->invoke.arity = arity;
    fn->invoke.name = name;
    return fn;
}

bool call(ObjCallable *callable, CallFrame *frame){
    frame->fn = callable;
    frame->error = NULL;
    switch(callable->type){
        case FN:{
            ObjFn *fn = callable;
            frame->ip = fn->chunk.code;
            frame->chunk = &fn->chunk;
            return true;
        }
        case NAT_FN:{
            ObjNativeFn *fn = callable;
            Value result = fn->fn(frame);
            if(frame->error) return false;
            frame->slots[0] = result;
            return true;
        }
    }
}