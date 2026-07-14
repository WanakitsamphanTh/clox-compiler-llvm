#include "vm.h"
#include "value.h"
#include "obj.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "function.h"
#include "natfn.h"
#include "compiler/resolve_scope.h"

static uint8_t vmReadByte(VM*);
static Value vmReadConstant(VM*);
static uint16_t vmReadShort(VM*);
static void resetStack(VM*);
static void resetFrameCall(VM*);
static Value vmPeek(VM*, size_t);
static void defineNative(void*, const char* name, int arity, NativeFn fn);
static ObjUpValue* vmCaptureValue(VM*, Value*);
static void vmCloseUpValue(VM*, Value*);
static void vmCloseCallFrame(VM*);

void initVM(VM* vm){
    resetStack(vm);
    initObjHeap(&vm->heap);
    vm->heap.owner = vm;
    initTable(&vm->globals);
    initGC(&vm->gc);
    defineNativeFunctions(vm, &defineNative);
    vm->call_frames = malloc(sizeof(CallFrame) * FRAME_MAX);
    if(vm->call_frames == NULL){
        printf("cannot allocate call_frames\n");
        exit(1);
    }
    vm->frame = vm->call_frames;
    vm->frame->slots = vm->stack_top;
    vm->frame_count = 1;
    vm->has_error = false;
    vm->upvalues = NULL;
}

void freeVM(VM* vm){
    freeObjHeap(&vm->heap);
    freeGC(&vm->gc);
    free(vm->call_frames);
    if(vm->upvalues != NULL){
        UpValueNode* node;
        UpValueNode* next;
        node = vm->upvalues;
        while(node != NULL){
            next = node->next;
            free(node);
            node = next;
        }
    }
}

InterpretResult vmInterpret(VM* vm, Chunk* chunk) {
    vm->frame->chunk = chunk;
    vm->frame->ip = vm->frame->chunk->code;
    return runVM(vm);
}

#define ARITH_BINARY_OP(op) do { \
    Value b = vmPeek(vm, 0); \
    Value a = vmPeek(vm, 1); \
    if(!IS_NUM(a) || !IS_NUM(b)){ \
        runtimeError(vm, "Operands must be both number & number"); \
        return INTERPRET_RUNTIME_ERROR; \
    } \
    Value c = VALUE_NUM(AS_NUM(a) op AS_NUM(b)); \
    vmPop(vm); \
    vmPop(vm); \
    vmPush(vm, c); \
    } while(0)

#define COMP_BINARY_OP(op) do { \
    Value b = vmPeek(vm, 0); \
    Value a = vmPeek(vm, 1); \
    if(!IS_NUM(a) || !IS_NUM(b)){ \
        runtimeError(vm, "Operands must be both number & number"); \
        return INTERPRET_RUNTIME_ERROR; \
    } \
    Value c = VALUE_BOOL(AS_NUM(a) op AS_NUM(b)); \
    vmPop(vm); \
    vmPop(vm); \
    vmPush(vm, c); \
    } while(0)

#define BOOL_BINARY_OP(op) do { \
    Value b = vmPeek(vm, 0); \
    Value a = vmPeek(vm, 1); \
    Value c = VALUE_BOOL(IS_TRUTHY(a) op IS_TRUTHY(b)); \
    vmPop(vm); \
    vmPop(vm); \
    vmPush(vm, c); \
    } while(0)

InterpretResult runVM(VM* vm){
    uint8_t instruction;
    Value value;
    char buffer[256];
    
    while(1){
        instruction = vmReadByte(vm);
        switch(instruction){
            case OP_CONST:
                value = vmReadConstant(vm);
                vmPush(vm, value);
                break;

            case OP_TRUE: 
                vmPush(vm, VALUE_BOOL(true));
                break;
            case OP_FALSE: 
                vmPush(vm, VALUE_BOOL(false));
                break;
            case OP_NIL: 
                vmPush(vm, VALUE_NIL);
                break;
            case OP_ARR:{
                uint8_t size = vmReadByte(vm);

                Value* v_arr = malloc(sizeof(Value)*size);
                uint8_t i;

                // poping value for array
                for(i = 0; i < size; i++)
                    v_arr[i] = vmPeek(vm, i);
                
                Value array = VALUE_OBJ(makeObjArray(&vm->heap, size, v_arr));

                for(i = 0; i < size; i++)
                    vmPop(vm);

                vmPush(vm, array);

                free(v_arr);
                break;
            }

            case OP_NEGATE:
                if(vmPeek(vm, 0).type != NUMBER_VALUE){
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                } else {
                    value = vmPop(vm);
                    value.val.num = -value.val.num;
                    vmPush(vm, value); 
                }
                break;
            case OP_ADD:{
                if(IS_STR(vmPeek(vm, 1))){
                    Value b = vmPeek(vm, 0);
                    Value a = vmPeek(vm, 1);
                    ObjString *first = AS_STR(a);
                    ObjString *second;

                    if(!IS_STR(b)){
                        second = valueToObjString(&vm->heap, b);
                    } else {
                        second = AS_STR(b);
                    }
                    Value c;
                    c.type = OBJ_VALUE;
                    c.val.obj = concatObjString(&vm->heap, first, second);
                    vmPop(vm);
                    vmPop(vm);
                    vmPush(vm, c);
                } else if(IS_ARRAY(vmPeek(vm, 1)) && IS_ARRAY(vmPeek(vm, 0))){
                    Value b = vmPeek(vm, 0);
                    Value a = vmPeek(vm, 1);
                    ObjArray *array = concatObjArray(&vm->heap, AS_OBJ(a), AS_OBJ(b));
                    vmPop(vm);
                    vmPop(vm);
                    vmPush(vm, VALUE_OBJ(array));
                } else
                    ARITH_BINARY_OP(+);
                break;
            }
            case OP_SUB:
                ARITH_BINARY_OP(-); break;
            case OP_MULT:
                ARITH_BINARY_OP(*); break;
            case OP_DIV:
                ARITH_BINARY_OP(/); break;
            case OP_MOD:{ 
                    Value b = vmPeek(vm, 0);
                    Value a = vmPeek(vm, 1);
                    if(!IS_NUM(a) || !IS_NUM(b)){ 
                        runtimeError(vm, "Operands must be both number & number"); 
                        return INTERPRET_RUNTIME_ERROR; 
                    }
                    Value c = VALUE_NUM(((long long) AS_NUM(a)) % ((long long) AS_NUM(b))); 
                    vmPop(vm);
                    vmPop(vm);
                    vmPush(vm, c); 
                }
                break;
            case OP_AND: 
                BOOL_BINARY_OP(&&); break;
            case OP_OR:
                BOOL_BINARY_OP(||); break;
            case OP_CMPL:
                value = vmPop(vm);
                vmPush(vm, VALUE_BOOL(!IS_TRUTHY(value))); 
                break;
            case OP_LESS:
                COMP_BINARY_OP(<); break;
            case OP_LESS_EQ:
                COMP_BINARY_OP(<=); break;
            case OP_GREATER:
                COMP_BINARY_OP(>); break;
            case OP_GREATER_EQ:
                COMP_BINARY_OP(>=); break;
            case OP_EQ:{
                Value b = vmPeek(vm, 0);
                Value a = vmPeek(vm, 1);
                Value c; 
                if(a.type == NIL_VALUE && b.type == NIL_VALUE)
                    c = VALUE_BOOL(true);
                else if(a.type == OBJ_VALUE  && b.type == OBJ_VALUE){
                    if(a.val.obj->type == OBJ_STRING && b.val.obj->type == OBJ_STRING)
                        c = VALUE_BOOL(strcmp(AS_CSTR(a), AS_CSTR(b)) == 0);
                    else c = VALUE_BOOL(false);
                } else if(a.type == NUMBER_VALUE && b.type == NUMBER_VALUE) {
                    c = VALUE_BOOL(AS_NUM(a) == AS_NUM(b));
                } else if(a.type == BOOL_VALUE && b.type == BOOL_VALUE){
                    c = VALUE_BOOL(AS_BOOL(a) == AS_BOOL(b));
                } else {
                    c= VALUE_BOOL(false); 
                }
                vmPop(vm);
                vmPop(vm);
                vmPush(vm, c); 
                break;
            }

            case OP_JIF:{
                uint16_t offset = vmReadShort(vm);
                if(IS_FALSY(vmPeek(vm,0))) 
                    vm->frame->ip += offset;
                break;
            }

            case OP_JMP:{
                uint16_t offset = vmReadShort(vm);
                vm->frame->ip += offset;
                break;
            }

            case OP_LOOP:{
                uint16_t offset = vmReadShort(vm);
                vm->frame->ip -= offset;
                break;
            }

            case OP_PRINT:
                value = vmPop(vm);
                memset(buffer,0,256);
                valueToString(value, buffer, 255);
                printf("%s\n", buffer);
                break;
            
            case OP_DEFINE_GLOB:{
                ObjString* name = AS_STR(vmReadConstant(vm));
                Value init_val = vmPeek(vm, 0);
                tableSet(&vm->globals, name, init_val);
                vmPop(vm);
                break;
            }

            case OP_LOAD_GLOB:{
                ObjString* name = AS_STR(vmReadConstant(vm));
                Value val;
                if(!tableGet(&vm->globals, name, &val)) {
                    runtimeError(vm, "Unknown variable %s\n", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value copy = val;
                vmPush(vm, copy);
                break;
            }

            case OP_STORE_GLOB:{
                ObjString* name = AS_STR(vmReadConstant(vm));
                Value val = vmPeek(vm,0);
                /* if the key is new, it should be error*/
                if(tableSet(&vm->globals, name, val)) {             
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Unknown variable %s\n", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LOAD_UVAL:{
                ObjCallable* callable = vm->frame->fn;
                if(callable->type != CLOSURE){
                    runtimeError(vm, "Unable to load upvalues outside a closure\n");
                    return INTERPRET_ERROR;
                }
                int index = vmReadByte(vm);
                ObjClosure* closure = (ObjClosure*) callable;
                if(index >= closure->upvalue_count) {
                    runtimeError(vm, "Upvalue index out of bound\n");
                    return INTERPRET_ERROR;
                }
                vmPush(vm, getUpValue(closure->upvalues[index]));
                break;
            }
            
            case OP_LOAD_LOC:{
                uint8_t slot = vmReadByte(vm);
                Value val = vm->frame->slots[slot];
                vmPush(vm, val);
                break;
            }

            case OP_STORE_LOC:{
                uint8_t slot = vmReadByte(vm);
                Value *assignee = &vm->frame->slots[slot];
                *assignee = vmPeek(vm, 0);
                break;
            }

            case OP_STORE_UVAL:{
                ObjCallable* callable = vm->frame->fn;
                if(callable->type != CLOSURE){
                    runtimeError(vm, "Unable to load upvalues outside a closure\n");
                    return INTERPRET_ERROR;
                }
                int index = vmReadByte(vm);
                ObjClosure* closure = (ObjClosure*) callable;
                if(index >= closure->upvalue_count) {
                    runtimeError(vm, "Upvalue index out of bound\n");
                    return INTERPRET_ERROR;
                }
                setUpValue(closure->upvalues[index], vmPeek(vm, 0));
                break;
            }

            case OP_CLOSURE: {
                Value fn_value = vmReadConstant(vm);
                uint8_t upvalue_count = vmReadByte(vm);
                if(upvalue_count == 0) {
                    vmPush(vm, fn_value);
                } else {
                    ObjClosure* closure = newClosure(&vm->heap, AS_OBJ(fn_value), upvalue_count);
                    for(int i = 0; i < upvalue_count; i++){
                        UpValueType uval_type = vmReadByte(vm);
                        int index = vmReadByte(vm);
                        if(uval_type == UVAL_LOC){
                            ObjUpValue* upvalue = vmCaptureValue(vm, vm->frame->slots + index);
                            if(upvalue == NULL){
                                runtimeError(vm, "Unable to capture the value (upvalue object allocation failed)\n");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            closure->upvalues[i] = upvalue;
                        }
                        else {
                            if(vm->frame->fn->type != CLOSURE){
                                runtimeError(vm, "Unable to use an upvalue from a non-closure function\n");
                                return INTERPRET_ERROR;
                            }
                            closure->upvalues[i] = ((ObjClosure*)vm->frame->fn)->upvalues[index];
                        }
                    }
                    vmPush(vm, VALUE_OBJ(closure));
                }
                break;
            }
            case OP_CLOSE_UVAL:{
                Value* ref = vm->stack_top - 1;
                vmCloseUpValue(vm, ref);
                vmPop(vm);
                break;
            }

            case OP_CALL:{
                uint8_t argc = vmReadByte(vm);
                valueToString(vmPeek(vm, argc), buffer, 256);
                if(!IS_CALLABLE(vmPeek(vm, argc))) {
                    runtimeError(vm, "The object is not callable\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjCallable* fn = AS_OBJ(vmPeek(vm, argc));
                if(argc != fn->arity) {
                    runtimeError(vm, "Function %s needs %d parameters (only %d given)\n", fn->name->str, fn->arity, argc);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(vm->frame_count == FRAME_MAX){
                    runtimeError(vm, "Stack overflow\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm->frame = &vm->call_frames[vm->frame_count++];
                vm->frame->slots = vm->stack_top - argc;
                if(!call(fn, vm)){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_IND:{
                Value index = vmPeek(vm, 0);
                Value array = vmPeek(vm, 1);
                if(!isObjType(array, OBJ_ARRAY)){
                    runtimeError(vm, "Cannot index-access a non-array type");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = (ObjArray*)AS_OBJ(array);
                int ind = AS_NUM(index);
                if(ind > arr->len - 1){
                    runtimeError(vm, "Index out of range");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPop(vm);
                vmPop(vm);
                vmPush(vm, arr->elements[ind]);
                break;
            }
            case OP_SET_IND:{
                Value index = vmPeek(vm, 0);
                Value array = vmPeek(vm, 1);
                if(!isObjType(array, OBJ_ARRAY)){
                    runtimeError(vm, "Cannot index-access a non-array type");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = (ObjArray*)AS_OBJ(array);
                int ind = AS_NUM(index);
                if(ind > arr->len - 1){
                    runtimeError(vm, "Index out of range");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPop(vm);
                vmPop(vm);
                arr->elements[ind] = vmPeek(vm, 0);
                break;
            }

            case OP_POP:
                vmPop(vm);
                break;

            case OP_RETURN: {
                vm->frame_count--;
                if(vm->frame_count == 0)
                    return INTERPRET_OK;
                Value result = vmPop(vm);
                vmCloseCallFrame(vm);
                vm->stack_top = vm->frame->slots - 1;
                vm->frame = &vm->call_frames[vm->frame_count - 1];
                vmPush(vm, result);
                break;
            }
        
            default:
                runtimeError(vm, "Unknown opcode %d\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }
}

void vmPush(VM* vm, Value value){
    *(vm->stack_top++) = value;
}

Value vmPop(VM* vm){
    return *(--vm->stack_top);
}

uint8_t vmReadByte(VM* vm){
    return *(vm->frame->ip++);
}

uint16_t vmReadShort(VM* vm){
    vm->frame->ip += 2;
    return (uint16_t)((vm->frame->ip[-2] << 8) | vm->frame->ip[-1] & 0xffff);
}

Value vmReadConstant(VM* vm) {
    Value value = vm->frame->chunk->constants.values[vmReadByte(vm)];
    return value;
}

void resetStack(VM* vm) {
    vm->stack_top = vm->stack;
}

void resetFrameCall(VM* vm){
    vm->frame = vm->call_frames;
    vm->frame_count = 1;
}

static Value vmPeek(VM* vm, size_t dist){
    return *(vm->stack_top - 1 - dist);
}

void defineNative(void* vm_ptr, const char* name, int arity, NativeFn fn){
    VM* vm = vm_ptr;
    size_t len = strlen(name);
    ObjString* fn_name = makeObjString(&vm->heap, name, len);
    //printf("define native: %s", fn_name->str);
    Value fn_val = VALUE_OBJ(newNativeFunction(&vm->heap, fn_name, arity, fn));
    tableSet(&vm->globals, fn_name, fn_val);
    Value val;
    tableGet(&vm->globals, fn_name, &val);
}

ObjUpValue* vmCaptureValue(VM* vm, Value* ref){
    // to implement
    // if found, return the value
    // if not found, return a new upvalue
    UpValueNode *prev, *current;
    prev = NULL;
    current = vm->upvalues;
    while(current != NULL && current->ref > ref){
        prev = current;
        current = current->next;
    }
    if(current != NULL && current->ref == ref) return current->upvalue;

    UpValueNode* node = malloc(sizeof(UpValueNode));
    node->ref = ref;
    node->upvalue = newUpValue(&vm->heap, ref);
    node->next = current;

    if(prev == NULL) 
        vm->upvalues = node;
    else 
        prev->next = node;

    return node->upvalue;
}

void vmCloseUpValue(VM* vm, Value* ref){
    UpValueNode *prev, *current;
    prev = NULL;
    current = vm->upvalues;
    while(current != NULL && current->ref > ref){
        prev = current;
        current = current->next;
    }
    if(current != NULL && current->ref == ref) {
        if(prev == NULL){
            vm->upvalues = current->next;
        } else {
            prev->next = current->next;
        }
        closeUpValue(current->upvalue);
        free(current);
    }
}

static void vmCloseCallFrame(VM* vm){
    Value* first = vm->frame->slots;
    while(vm->upvalues != NULL && vm->upvalues->ref >= first){
        UpValueNode* node = vm->upvalues;
        vm->upvalues = node->next;
        closeUpValue(node->upvalue);
        free(node);
    }
}

void runtimeError(VM* vm, const char* fmt, ...){
    va_list args;
    vm->has_error = true;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
    if(vm->frame_count != 1){
        CallFrame* frame = vm->frame;
        while(1){
            if(frame == vm->call_frames) {
                fprintf(stderr, "from script");
                break;
            }
            fprintf(stderr, "from %s()\n", frame->fn->name->str);
            frame--;
        }
    }
    resetFrameCall(vm);
    resetStack(vm);
}