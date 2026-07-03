#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
    Table strings;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult vmInterpret(Chunk* chunk);
InterpretResult runVM();
void vmPush(Value value);
Value vmPop();

extern VM vm;

#endif