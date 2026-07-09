#include "function.h"
#include "string.h"
#include "stdio.h"
#include "value.h"

void free_obj_fn(void* obj){
    ObjFn* fn = obj;
    freeChunk(&fn->chunk);
}

ObjCallable* newFunction(FunctionType type, int arity, ObjString* name, Value* (*_call)(Value*)){
    ObjCallable* fn;
    switch(type){
        case NAT_FN:
            fn = AllocateObj(OBJ_FN, NULL, sizeof(ObjNativeFn));
            fn->call = _call;
            break;
        case FN:{
            fn = AllocateObj(OBJ_FN, &free_obj_fn, sizeof(ObjFn));
            fn->call = _fn_call;
            ObjFn* obj_fn = fn;
            initChunk(&obj_fn->chunk);
            break;
        }
    }
    fn->arity = arity;
    fn->name = name;
    return fn;
}


Value _fn_call(ErrorHandler* _, Value* args){
    // user-defined functions do not need error handler yet
}

Value _nat_scan(ErrorHandler* handler, Value* _){
    char buffer[256];
    scanf("%s", buffer);
    int len = stdlen(buffer);
    return VALUE_OBJ(makeObjString(buffer, len));
}

Value _nat_scan_ln(ErrorHandler* handler, Value* _){
    char buffer[256];
    scanf("%s", buffer);
    gets_s(buffer, 256);
    int len = stdlen(buffer);
    return VALUE_OBJ(makeObjString(buffer, len));
}

Value _nat_scan_num(ErrorHandler* handler, Value* _){
    double val;
    scanf("%lf", &val);
    return VALUE_NUM(val);
}

Value _nat_clock(ErrorHandler* handler, Value* _){
    time_t now;
    time(&now); 
    return VALUE_NUM(now);
}

Value _nat_map(ErrorHandler* handler, Value* args){
    if(args[0].type != OBJ_VALUE || args[1].type != OBJ_VALUE) return VALUE_NIL;
    Obj* arr_obj = args[0].val.obj;
    Obj* fn_obj = args[1].val.obj;
    if(arr_obj->type != OBJ_ARRAY || fn_obj->type != OBJ_FN) return VALUE_NIL;
    ObjArray* array = arr_obj;
    ObjCallable* fn = fn_obj;

    return VALUE_NIL;
}
