#include "vm.h"
#include "value.h"
#include <stddef.h>
#include <stdio.h>

VM vm;

static uint8_t readByte();
static Value vmReadConstant();
static void resetStack();

void initVM(){
    resetStack();
}

void freeVM(){}

InterpretResult vmInterpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return runVM();
}

#define ARITH_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    a.val.num = a.val.num op b.val.num; \
    vmPush(a); \
    } while(0)

#define COMP_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    a.val.b = a.val.num op b.val.num; \
    a.type = BOOL_VALUE; \
    vmPush(a); \
    } while(0)

#define BOOL_BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    a.val.b = a.val.b op b.val.b; \
    vmPush(a); \
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
                value = vmPop();
                value.val.num = -value.val.num;
                vmPush(value); 
                break;
            case OP_ADD:
                ARITH_BINARY_OP(+); break;
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
                value.val.b = !value.val.b;
                vmPush(value); 
                break;
            case OP_LESS:
                COMP_BINARY_OP(<); break;
            case OP_LESS_EQ:
                COMP_BINARY_OP(<=); break;
            case OP_GREATER:
                COMP_BINARY_OP(>); break;
            case OP_GREATER_EQ:
                COMP_BINARY_OP(>=); break;
            case OP_EQ:
                COMP_BINARY_OP(==); break;

            case OP_PRINT:
                value = vmPop();
                valueToString(value, buffer);
                printf("%s\n", buffer);
                break;

            default:
                fprintf(stderr, "Unknown opcode %d\n", instruction);
                return INTERPRET_ERROR;
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

Value vmReadConstant() {
    Value value = vm.chunk->constants.values[readByte()];
    return value;
}

void resetStack() {
    vm.stackTop = vm.stack;
}