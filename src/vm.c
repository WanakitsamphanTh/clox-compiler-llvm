#include "vm.h"
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

#define BINARY_OP(op) do { \
    Value b = vmPop(); \
    Value a = vmPop(); \
    a.val.num = a.val.num op b.val.num; \
    vmPush(a); \
    } while(0)

InterpretResult runVM(){
    uint8_t instruction;
    Value value;

    while(1){
        instruction = readByte();
        switch(instruction){
            case OP_CONST:
                value = vmReadConstant();
                vmPush(value);
                break;
            case OP_RETURN:
                value = vmPop();
                printf("%.8f\n", value.val.num);
                return INTERPRET_OK;
            case OP_NEGATE:
                value = vmPop();
                value.val.num = - value.val.num;
                vmPush(value); 
                break;
            case OP_ADD:
                BINARY_OP(+); break;
            case OP_SUB:
                BINARY_OP(-); break;
            case OP_MULT:
                BINARY_OP(*); break;
            case OP_DIV:
                BINARY_OP(/); break;
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