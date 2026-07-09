#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"
#include "function.h"

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * (UINT8_MAX + 1))

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value* stack_top;
    size_t frame_count;
    Obj* objects;
    Table strings;
    Table globals;
    Value stack[STACK_MAX];
    CallFrame call_frame[FRAME_MAX];
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