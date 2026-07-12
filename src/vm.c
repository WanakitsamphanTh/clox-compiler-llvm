#include "vm.h"
#include "value.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "function.h"
#include "natfn.h"

VM vm;

static void freeObjects();

static uint8_t vmReadByte();
static Value vmReadConstant();
static uint16_t vmReadShort();
static void resetStack();
static Value vmPeek(size_t);
static void RuntimeError(const char* fmt, ...);
static void defineNative(void*, const char* name, int arity, NativeFn fn);

void initVM(){
    resetStack();
    initTable(&vm.strings);
    initTable(&vm.globals);
    vm.objects = NULL;
    vm.call_frames = malloc(sizeof(CallFrame) * FRAME_MAX);
    vm.frame = vm.call_frames;
    vm.frame->slots = vm.stack_top;
    vm.frame_count = 1;
}

void freeVM(){
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
    free(vm.call_frames);
}

void freeObjects(){
    Obj* obj = vm.objects;
    Obj* next;
    while(obj){
        next = obj->next;
        freeObj(obj);
        obj = next;
    }
}

InterpretResult vmInterpret(Chunk* chunk) {
    vm.frame->chunk = chunk;
    vm.frame->ip = vm.frame->chunk->code;
    defineNativeFunctions(&vm, &defineNative);
    return runVM();
}

#define ARITH_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    if(!IS_NUM(a) || !IS_NUM(b)){ \
        RuntimeError("Operands must be both number & number"); \
        return INTERPRET_RUNTIME_ERROR; \
    } \
    Value c = VALUE_NUM(AS_NUM(a) op AS_NUM(b)); \
    vmPush(c); \
    } while(0)

#define COMP_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    if(!IS_NUM(a) || !IS_NUM(b)){ \
        RuntimeError("Operands must be both number & number"); \
        return INTERPRET_RUNTIME_ERROR; \
    } \
    Value c = VALUE_BOOL(AS_NUM(a) op AS_NUM(b)); \
    vmPush(c); \
    } while(0)

#define BOOL_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    Value c = VALUE_BOOL(IS_TRUTHY(a) op IS_TRUTHY(b)); \
    vmPush(c); \
    } while(0)

InterpretResult runVM(){
    uint8_t instruction;
    Value value;
    char buffer[256];
    
    while(1){
        instruction = vmReadByte();
        switch(instruction){
            case OP_CONST:
                value = vmReadConstant();
                vmPush(value);
                break;

            case OP_TRUE: 
                vmPush(VALUE_BOOL(true));
                break;
            case OP_FALSE: 
                vmPush(VALUE_BOOL(false));
                break;
            case OP_NIL: 
                vmPush(VALUE_NIL);
                break;
            case OP_ARR:{
                uint8_t size = vmReadByte();

                Value* v_arr = malloc(sizeof(Value)*size);
                uint8_t i;

                // poping value for array
                for(i = 0; i < size; i++)
                    v_arr[i] = vmPop();
                
                vmPush(VALUE_OBJ(makeObjArray(size, v_arr)));

                free(v_arr);
                break;
            }

            case OP_NEGATE:
                if(vmPeek(0).type != NUMBER_VALUE){
                    RuntimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                } else {
                    value = vmPop();
                    value.val.num = -value.val.num;
                    vmPush(value); 
                }
                break;
            case OP_ADD:{
                if(IS_STR(vmPeek(1))){
                    Value b = vmPop();
                    Value a = vmPop();
                    ObjString *first = AS_STR(a);
                    ObjString *second;

                    if(!IS_STR(b)){
                        second = valueToObjString(b);
                    } else {
                        second = AS_STR(b);
                    }
                    Value c;
                    c.type = OBJ_VALUE;
                    c.val.obj = concatObjString(first, second);
                    vmPush(c);
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
            case OP_AND: 
                BOOL_BINARY_OP(&&); break;
            case OP_OR:
                BOOL_BINARY_OP(||); break;
            case OP_CMPL:
                value = vmPop();
                vmPush(VALUE_BOOL(!IS_TRUTHY(value))); 
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
                Value b = vmPop();
                Value a = vmPop();
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
                vmPush(c); 
                break;
            }

            case OP_JIF:{
                uint16_t offset = vmReadShort();
                if(IS_FALSY(vmPeek(0))) 
                    vm.frame->ip += offset;
                break;
            }

            case OP_JMP:{
                uint16_t offset = vmReadShort();
                vm.frame->ip += offset;
                break;
            }

            case OP_LOOP:{
                uint16_t offset = vmReadShort();
                vm.frame->ip -= offset;
                break;
            }

            case OP_PRINT:
                value = vmPop();
                memset(buffer,0,256);
                valueToString(value, buffer, 255);
                printf("%s\n", buffer);
                break;
            
            case OP_DEFINE_GLOB:{
                ObjString* name = AS_STR(vmReadConstant());
                Value init_val = vmPeek(0);
                tableSet(&vm.globals, name, init_val);
                vmPop();
                break;
            }

            case OP_LOAD_GLOB:{
                ObjString* name = AS_STR(vmReadConstant());
                Value val;
                if(!tableGet(&vm.globals, name, &val)) {
                    RuntimeError("Unknown variable %s\n", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value copy = val;
                vmPush(copy);
                break;
            }

            case OP_STORE_GLOB:{
                ObjString* name = AS_STR(vmReadConstant());
                Value val = vmPeek(0);
                /* if the key is new, it should be error*/
                if(tableSet(&vm.globals, name, val)) {             
                    tableDelete(&vm.globals, name);
                    RuntimeError("Unknown variable %s\n", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_LOAD_LOC:{
                uint8_t slot = vmReadByte();
                Value val = vm.frame->slots[slot];
                vmPush(val);
                break;
            }

            case OP_STORE_LOC:{
                uint8_t slot = vmReadByte();
                Value *assignee = &vm.frame->slots[slot];
                *assignee = vmPeek(0);
                break;
            }

            case OP_CALL:{
                uint8_t argc = vmReadByte();
                valueToString(vmPeek(argc), buffer, 256);
                
                printf("esp - argc [%d] : %s\n", argc, buffer);
                // to implement
                if(!IS_CALLABLE(vmPeek(argc))) {
                    RuntimeError("The object is not callable\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjCallable* fn = AS_OBJ(vmPeek(argc));
                if(argc != fn->arity) {
                    RuntimeError("Function %s needs %d parameters (only %d given)\n", fn->name->str, fn->arity, argc);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(vm.frame_count == FRAME_MAX){
                    RuntimeError("Stack overflow\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.frame = &vm.call_frames[vm.frame_count++];
                vm.frame->slots = vm.stack_top - argc;
                if(!call(fn, &vm)){
                    // currently error is implemented as string
                    ObjString* error = vm.frame->error;
                    RuntimeError("Error occured in %s\n\t%s\n", fn->name->str, error->str);
                    return INTERPRET_ERROR;
                }
                break;
            }

            case OP_POP:
                vmPop();
                break;

            case OP_RETURN: {
                vm.frame_count--;
                if(vm.frame_count == 0)
                    return INTERPRET_OK;
                Value result = vmPop();
                vm.stack_top = vm.frame->slots - 1;
                vm.frame = &vm.call_frames[vm.frame_count - 1];
                vmPush(result);
                break;
            }
        
            default:
                RuntimeError("Unknown opcode %d\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }
}

void vmPush(Value value){
    *(vm.stack_top++) = value;
}

Value vmPop(){
    return *(--vm.stack_top);
}

uint8_t vmReadByte(){
    return *(vm.frame->ip++);
}

uint16_t vmReadShort(){
    vm.frame->ip += 2;
    return (uint16_t)((vm.frame->ip[-2] << 8) | vm.frame->ip[-1] & 0xffff);
}

Value vmReadConstant() {
    Value value = vm.frame->chunk->constants.values[vmReadByte()];
    return value;
}

void resetStack() {
    vm.stack_top = vm.stack;
}

static Value vmPeek(size_t dist){
    return *(vm.stack_top - 1 - dist);
}

void RuntimeError(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
    resetStack();
}

void defineNative(void* vm_ptr, const char* name, int arity, NativeFn fn){
    VM* vm = vm_ptr;
    size_t len = strlen(name);
    ObjString* fn_name = makeObjString(name, len);
    printf("define native: %s", fn_name->str);
    Value fn_val = VALUE_OBJ(newNativeFunction(fn_name, arity, fn));
    tableSet(&vm->globals, fn_name, fn_val);
    Value val;
    tableGet(&vm->globals, fn_name, &val);
    char buffer[256];
    valueToString(val, buffer, 256);
    printf("\n%s\n", buffer);
}