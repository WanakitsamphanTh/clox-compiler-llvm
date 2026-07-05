#include "compiler/control_resolver.h"
#include "memory.h"
#include "stdlib.h"

LoopStack loop_stack;

void initLoopStack(){
    loop_stack.capacity = 0;
    loop_stack.count = 0;
    loop_stack.loops = malloc(0);
}

void freeLoopStack(){
    int i;
    for(i = 0; i < loop_stack.count; i++)
        freeLoop(&loop_stack.loops[i]);
    free(loop_stack.loops);
    loop_stack.count = 0;
    loop_stack.capacity = 0;
}

void pushLoop(){
    if(loop_stack.count + 1 > loop_stack.capacity) {
        size_t old_capacity = loop_stack.capacity;
        loop_stack.capacity = growCapacity(old_capacity);
        loop_stack.loops = growArray(sizeof(Loop), loop_stack.loops, old_capacity, loop_stack.capacity);
    }
    Loop* loop = &loop_stack.loops[loop_stack.count++];
    initJumpArray(&loop->break_list);
    initJumpArray(&loop->skip_list);
}

Loop popLoop(){
    return loop_stack.loops[--loop_stack.count];
}

Loop* topLoop(){
    if(loop_stack.count == 0) return NULL;
    return loop_stack.loops + loop_stack.count - 1;
}

void loopAddBreak(Loop* loop, int offset){
    appendJumpArray(&loop->break_list, offset);
}

void loopAddSkip(Loop* loop, int offset){
    appendJumpArray(&loop->skip_list, offset);
}

void freeLoop(Loop* loop){
    freeJumpArray(&loop->break_list);
    freeJumpArray(&loop->skip_list);
}

void initJumpArray(JumpArray* arr){
    arr->capacity = 0;
    arr->count = 0;
    arr->jumps = 0;
}

void appendJumpArray(JumpArray* arr, int offset){
    if(arr->count + 1 > arr->capacity){
        size_t old_capacity = arr->capacity;
        arr->capacity = growCapacity(old_capacity);
        arr->jumps = growArray(sizeof(int), arr->jumps, old_capacity, arr->capacity);
    }
    arr->jumps[arr->count++] = offset;
}

void freeJumpArray(JumpArray* arr){
    free(arr->jumps);
    arr->capacity = 0;
    arr->count = 0;
}