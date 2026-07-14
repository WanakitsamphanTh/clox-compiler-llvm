#include "function.h"
#include <string.h>
#include <stdio.h>
#include "value.h"
#include "obj.h"
#include "vm.h"
#include "gc.h"

void free_obj_fn(Obj* obj);
void blacken_obj_fn(GC*, Obj* obj);
void blacken_obj_callable(GC*, Obj* obj);
void blacken_obj_closure(GC*, Obj* obj);

struct _Obj_Vtable obj_callable_vtable = {.destructor = NULL, .blacken = blacken_obj_callable};
struct _Obj_Vtable obj_fn_vtable = {.destructor = free_obj_fn, .blacken = blacken_obj_fn};
struct _Obj_Vtable obj_closure_vtable = {.destructor = NULL, .blacken = blacken_obj_closure};

ObjCallable* newFunction(ObjHeap* heap, ObjString* name, int arity){
    ObjFn* fn = AllocateObj(heap, OBJ_CALLABLE, &obj_fn_vtable, sizeof(ObjFn));
    fn->invoke.name = name;
    fn->invoke.arity = arity;
    fn->invoke.type = FN;
    fn->chunk = newChunk();
    return fn;
}
ObjCallable* newNativeFunction(ObjHeap* heap, ObjString* name, int arity, NativeFn callee){
    ObjNativeFn* fn = AllocateObj(heap, OBJ_CALLABLE, &obj_vtable, sizeof(ObjNativeFn));
    fn->fn = callee;
    fn->invoke.type = NAT_FN;
    fn->invoke.arity = arity;
    fn->invoke.name = name;
    return fn;
}

bool call(ObjCallable *callable, VM *vm){
    vm->frame->fn = callable;
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
            if(vm->has_error) return false;
            // clear stack
            vm->frame_count--;
            vm->stack_top = vm->frame->slots - 1;
            vm->frame = &vm->call_frames[vm->frame_count - 1];
            vmPush(vm, result);
            return success;
        }
        case CLOSURE:{
            ObjClosure* closure = callable;
            vm->frame->ip = closure->prototype->chunk.code;
            vm->frame->chunk = &closure->prototype->chunk;
            return true;
        }
    }
}

ObjCallable* newClosure(ObjHeap* heap, ObjFn* fn, size_t upvalue_count){
    ObjClosure* closure = AllocateObj(heap, OBJ_CALLABLE, &obj_vtable, sizeof(ObjClosure) + sizeof(ObjUpValue*) * upvalue_count);
    closure->invoke.arity = fn->invoke.arity;
    closure->invoke.name = fn->invoke.name;
    closure->invoke.type = CLOSURE;
    closure->prototype = fn;
    closure->upvalue_count = upvalue_count;
    return closure;
}

ObjUpValue* newUpValue(ObjHeap* heap, Value* ref){
    ObjUpValue *uval = AllocateObj(heap, OBJ_UPVALUE, &obj_vtable, sizeof(ObjUpValue));
    uval->ref = ref;
    return uval;
}

void closeUpValue(ObjUpValue* upvalue){
    upvalue->value = *upvalue->ref;
    upvalue->ref = NULL;
}

Value getUpValue(ObjUpValue* upvalue){
    if(upvalue->ref != NULL) return *upvalue->ref;
    else return upvalue->value;
}

void setUpValue(ObjUpValue* upvalue, Value value){
    if(upvalue->ref != NULL) *upvalue->ref = value;
    else upvalue->value = value;
}

void free_obj_fn(Obj* obj){
    ObjFn* fn = obj;
    freeChunk(&fn->chunk);
}

void blacken_obj_callable(GC* gc, Obj* obj){
    ObjCallable* callable = obj;
    gcMarkObj(gc, callable->name);
}

void blacken_obj_fn(GC* gc, Obj* obj){
    blacken_obj_callable(gc, obj);
    ObjFn *fn = obj;
    gcMarkChunk(gc, &fn->chunk);
}

void blacken_obj_closure(GC* gc, Obj* obj){
    blacken_obj_callable(gc, obj);
    ObjClosure* closure = obj;
    // mark the prototype
    gcMarkObj(gc, closure->prototype);
    for(int i = 0; i < closure->upvalue_count; i++){
        // mark upvalues
        gcMarkObj(gc, closure->upvalues[i]);
    }
}