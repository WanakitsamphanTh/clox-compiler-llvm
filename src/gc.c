#include "gc.h"
#include "function.h"

void gcMarkValue(Value* value){
    if(value->type != OBJ_VALUE) return;
    gcMarkObj(value->val.obj);
}

void gcMarkObj(Obj* obj){
    obj->vtable->mark(obj);
}

void gcMarkTable(Table* table){
    for(int i = 0; i < table->count; i++){
        Entry* entry = &table->entries[i];
        Obj* key = entry->key;
        gcMarkObj(key);
        gcMarkValue(&entry->value);
    }
}

void gcMarkChunk(Chunk* chunk){
    for(int i = 0; i < chunk->constants.count; i++){
        gcMarkValue(&chunk->constants.values[i]);
    }
}

void gcMarkRoots(VM* vm){
    // mark call frames
    for(int i = 0; i < vm->frame_count; i++)
        gcMarkObj(vm->call_frames[i].fn);

    // mark stack
    for(Value* sp = vm->stack; sp <= vm->stack_top; sp++)
        gcMarkValue(sp);
    
    // mark global table
    gcMarkTable(&vm->globals);

    // mark string pool
    //gcMarkTable(&vm->heap.strings);
}

void gcSweep(ObjHeap* heap){
    Obj* objects = heap->objects;
}