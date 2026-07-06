#ifndef RESOLVE_LOOP_H
#define RESOLVE_LOOP_H
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

void initLoopStack(LoopStack*);
void freeLoopStack(LoopStack*);
void pushLoop(LoopStack*);
Loop popLoop(LoopStack*);
Loop* topLoop(LoopStack*);

void loopAddBreak(Loop*, int);
void loopAddSkip(Loop*, int);
void freeLoop(Loop*);

void initJumpArray(JumpArray*);
void appendJumpArray(JumpArray*, int);
void freeJumpArray(JumpArray*);

#endif