#include "vm.h"
#include "value.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

VM vm;

static void freeObjects();

static uint8_t readByte();
static Value vmReadConstant();
static uint16_t vmReadShort();
static void resetStack();
static Value vmPeek(size_t);
static void RuntimeError(const char* fmt, ...);

void initVM(){
    resetStack();
    initTable(&vm.strings);
    initTable(&vm.globals);
    vm.objects = NULL;
}

void freeVM(){
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
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
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
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
        instruction = readByte();
        switch(instruction){
            case OP_CONST:
                value = vmReadConstant();
                vmPush(value);
                break;
            case OP_RETURN:
                return INTERPRET_OK;

            case OP_TRUE: 
                vmPush(VALUE_BOOL(true));
                break;
            case OP_FALSE: 
                vmPush(VALUE_BOOL(false));
                break;
            case OP_NIL: 
                vmPush(VALUE_NIL);
                break;
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
                if(a.type == OBJ_VALUE  && b.type == OBJ_VALUE){
                    if(a.val.obj->type == OBJ_STRING && b.val.obj->type == OBJ_STRING)
                        c = VALUE_BOOL(strcmp(AS_CSTR(a), AS_CSTR(b)) == 0);
                    else c = VALUE_BOOL(false);
                } else if(a.type == NUMBER_VALUE && b.type == NUMBER_VALUE) {
                    c = VALUE_BOOL(AS_NUM(a) == AS_NUM(b));
                } else if(a.type == BOOL_VALUE && b.type == BOOL_VALUE){
                    c = VALUE_BOOL(AS_BOOL(a) == AS_BOOL(b));
                }
                else {
                    c= VALUE_BOOL(false); 
                }
                vmPush(c); 
                break;
            }

            case OP_JIF:{
                uint16_t offset = vmReadShort();
                if(IS_FALSY(vmPeek(0))) 
                    vm.ip += offset;
                break;
            }

            case OP_JMP:{
                uint16_t offset = vmReadShort();
                vm.ip += offset;
                break;
            }

            case OP_LOOP:{
                uint16_t offset = vmReadShort();
                vm.ip -= offset;
                break;
            }

            case OP_PRINT:
                value = vmPop();
                valueToString(value, buffer);
                printf("%s\n", buffer);
                break;
            

            case OP_DEFINE_GLOBAL:{
                ObjString* name = AS_STR(vmReadConstant());
                Value init_val = vmPeek(0);
                valueToString(init_val, buffer);
                tableSet(&vm.globals, name, init_val);
                vmPop();
                break;
            }

            case OP_LOAD_GLOBAL:{
                ObjString* name = AS_STR(vmReadConstant());
                Value val;
                if(!tableGet(&vm.globals, name, &val)) {
                    RuntimeError("Unknown variable %s\n", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                valueToString(val, buffer);
                vmPush(val);
                break;
            }

            case OP_STORE_GLOBAL:{
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

            case OP_POP:
                vmPop();
                break;
            
            default:
                RuntimeError("Unknown opcode %d\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }
}

void vmPush(Value value){
    *(vm.stackTop++) = value;
}

Value vmPop(){
    return *(--vm.stackTop);
}

uint8_t readByte(){
    return *(vm.ip++);
}

uint16_t vmReadShort(){
    vm.ip += 2;
    return (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1] & 0xffff);
}

Value vmReadConstant() {
    Value value = vm.chunk->constants.values[readByte()];
    return value;
}

void resetStack() {
    vm.stackTop = vm.stack;
}

static Value vmPeek(size_t dist){
    return *(vm.stackTop - 1 - dist);
}

void RuntimeError(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
    resetStack();
}