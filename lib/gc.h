#ifndef GC_H
#define GC_H
#include "common.h"
#include "value.h"
#include "obj.h"
#include "table.h"

typedef struct _VM VM;
typedef struct _Chunk Chunk;

typedef struct _GC {
    size_t gray_count;
    size_t gray_capacity;
    Obj** gray_stack;
    bool enabled;
} GC;

void gcMarkValue(GC*, Value*);
void gcMarkObj(GC*, Obj*);
void gcMarkTable(GC*, Table*);
void gcMarkChunk(GC*, Chunk*);
void gcMarkRoots(VM*);
void gcSweep(ObjHeap*);

void initGC(GC*);
void freeGC(GC*);
void gcPushGray(GC*, Obj*);
Obj* gcPopGray(GC*);

void gcBlackenObj(GC*, Obj*);
void gcCollect(VM*);

extern GC gc;

#endif