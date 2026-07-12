#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * (UINT8_MAX + 1))

typedef struct _CallFrame CallFrame;

typedef struct _VM {
    //Chunk* chunk;
    //uint8_t* ip;
    Value* stack_top;
    size_t frame_count;
    CallFrame* frame;
    //Obj* objects;
    //Table strings;
    Table globals;
    ObjHeap heap;
    Value stack[STACK_MAX];
    CallFrame* call_frames;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(VM*);
void freeVM(VM*);
InterpretResult vmInterpret(VM* vm, Chunk* chunk);
InterpretResult runVM(VM* vm);
void vmPush(VM* vm, Value value);
Value vmPop(VM* vm);

#endif