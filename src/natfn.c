#include "natfn.h"
#include "value.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

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

void defineNativeFunctions(void* instance, void (*defineNative)(void*, const char*, int, NativeFn)){
    defineNative(instance, "scanNumber", 0, &_nat_scan_num);
    defineNative(instance, "scan", 0, &_nat_scan);
    defineNative(instance, "scanLn", 0, &_nat_scan_ln);
    defineNative(instance, "clock", 0, &_nat_clock);
}
