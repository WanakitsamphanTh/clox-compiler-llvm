#ifndef CONTROL_RESOLVER_H
#define CONTROL_RESOLVER_H
#include "common.h"

typedef struct {
    size_t count;
    size_t capacity;
    int* jumps;
} JumpArray;

/* loop */
typedef struct {
    JumpArray break_list;    // collect jump for break
    JumpArray skip_list; // collect jump for skip
} Loop;

typedef struct {
    size_t count;
    size_t capacity;
    Loop *loops;
} LoopStack;

extern LoopStack loop_stack;

void initLoopStack();
void freeLoopStack();
void pushLoop();
Loop popLoop();
Loop* topLoop();

void loopAddBreak(Loop*, int);
void loopAddSkip(Loop*, int);
void freeLoop(Loop*);

void initJumpArray(JumpArray*);
void appendJumpArray(JumpArray*, int);
void freeJumpArray(JumpArray*);

#endif