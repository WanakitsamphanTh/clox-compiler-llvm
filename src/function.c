#include "function.h"
#include <string.h>
#include <stdio.h>
#include "value.h"
#include "vm.h"

void free_obj_fn(void* obj){
    ObjFn* fn = obj;
    freeChunk(&fn->chunk);
}

ObjCallable* newFunction(ObjHeap* heap, ObjString* name, int arity){
    ObjFn* fn = AllocateObj(heap, OBJ_CALLABLE, NULL, sizeof(ObjFn));
    fn->invoke.name = name;
    fn->invoke.arity = arity;
    fn->invoke.type = FN;
    fn->chunk = newChunk();
    fn->invoke.obj.destructor = &free_obj_fn;
    return fn;
}
ObjCallable* newNativeFunction(ObjHeap* heap, ObjString* name, int arity, NativeFn callee){
    ObjNativeFn* fn = AllocateObj(heap, OBJ_CALLABLE, NULL, sizeof(ObjNativeFn));
    fn->fn = callee;
    fn->invoke.type = NAT_FN;
    fn->invoke.arity = arity;
    fn->invoke.name = name;
    return fn;
}

bool call(ObjCallable *callable, VM *vm){
    vm->frame->fn = callable;
    vm->frame->error = NULL;
    switch(callable->type){
        case FN:{
            ObjFn *fn = callable;
            vm->frame->ip = fn->chunk.code;
            vm->frame->chunk = &fn->chunk;
            return true;
        }
        case NAT_FN:{
            // call function
            ObjNativeFn *fn = callable;
            Value result = fn->fn(vm);
            bool success = true;
            if(vm->frame->error) success = false;
            // clear stack
            vm->frame_count--;
            vm->stack_top = vm->frame->slots - 1;
            vm->frame = &vm->call_frames[vm->frame_count - 1];
            vmPush(vm, result);
            return success;
        }
    }
}