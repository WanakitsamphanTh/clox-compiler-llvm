#include "natfn.h"
#include "value.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "math.h"

Value _nat_scan(VM* vm){
    char buffer[256];
    scanf("%s", buffer);
    ObjString* input = makeObjString(&vm->heap, buffer, strlen(buffer));
    return VALUE_OBJ(input);
}

Value _nat_scan_ln(VM* vm){
    char buffer[256];
    fgets(buffer, 256, stdin);
    ObjString* input = makeObjString(&vm->heap, buffer, strlen(buffer));
    return VALUE_OBJ(input);
}

Value _nat_scan_num(VM* vm){
    char buffer[64];
    Value input = VALUE_NUM(0);
    scanf("%s", buffer);
    if(sscanf(buffer, "%lf", &input.val.num) != 1){
        char error_msg[256];
        int n = sprintf(error_msg, "Invalid input: %s", buffer);
        vm->frame->error = makeObjString(&vm->heap, error_msg, n);
    }
    return input;
}
Value _nat_clock(VM* vm){
    time_t t = time(NULL);
    return VALUE_NUM(t);
}

Value _nat_range(VM* vm){
    Value _start = vm->frame->slots[0];
    if(!IS_NUM(_start)) {
        vm->frame->error = makeObjString(&vm->heap, "start must be number", 21);
        return VALUE_NIL;
    }
    double start = AS_NUM(_start);
    Value _end = vm->frame->slots[1];
    if(!IS_NUM(_end)){
        vm->frame->error = makeObjString(&vm->heap, "end must be number", 19);
        return VALUE_NIL;
    }
    double end = AS_NUM(_end);
    Value _inc = vm->frame->slots[2];
    if(!IS_NUM(_inc)){
        vm->frame->error = makeObjString(&vm->heap, "increment must be number", 23);
        return VALUE_NIL;
    }
    double inc = AS_NUM(_inc);
    int len = (end - start ) / inc;
    Value *v_arr = malloc(len * sizeof(Value));
    for(int i = 0; i < len; i++)
        v_arr[i] = VALUE_NUM(start + inc * i);
    ObjArray* array = makeObjArray(&vm->heap, len, v_arr);
    free(v_arr);
    return VALUE_OBJ(array);
}

Value _nat_len(VM* vm){
    Value arg = vm->frame->slots[0];
    if(arg.type != OBJ_VALUE) return VALUE_NIL;
    switch(AS_OBJ(arg)->type){
        case OBJ_STRING:
            return VALUE_NUM(AS_STR(arg)->len);
        case OBJ_ARRAY:
            return VALUE_NUM(AS_ARRAY(arg)->len);
        default:
            return VALUE_NIL;
    }
}

Value _nat_pow(VM* vm){
    Value _base = vm->frame->slots[0];
    if(!IS_NUM(_base)){
        vm->frame->error = makeObjString(&vm->heap, "base must be number", 20);
        return VALUE_NIL;
    }
    double base = AS_NUM(_base);
    Value _exp = vm->frame->slots[1];
    if(!IS_NUM(_exp)){
        vm->frame->error = makeObjString(&vm->heap, "exponential must be number", 27);
        return VALUE_NIL;
    }
    double exp = AS_NUM(_exp);
    double result = pow(base, exp);
    return VALUE_NUM(result);
}

void defineNativeFunctions(void* instance, void (*defineNative)(void*, const char*, int, NativeFn)){
    defineNative(instance, "scanNumber", 0, &_nat_scan_num);
    defineNative(instance, "scan", 0, &_nat_scan);
    defineNative(instance, "scanLn", 0, &_nat_scan_ln);
    defineNative(instance, "clock", 0, &_nat_clock);
    defineNative(instance, "range", 3, &_nat_range);
    defineNative(instance, "len", 1, &_nat_len);
    defineNative(instance, "pow", 2, &_nat_pow);
}