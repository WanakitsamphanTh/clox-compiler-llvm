#ifndef GC_H
#define GC_H
#include "common.h"
#include "value.h"
#include "obj.h"
#include "vm.h"
#include "table.h"

void gcMarkValue(Value*);
void gcMarkObj(Obj*);
void gcMarkTable(Table*);
void gcMarkChunk(Chunk*);
void gcMarkRoots(VM*);
void gcSweep(ObjHeap*);

#endif