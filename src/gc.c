#include "vm.h"
#include "chunk.h"
#include "gc.h"
#include "function.h"
#include "memory.h"
#include <stdio.h>

#define GC_HEAP_GROW_FACTOR 4

void gcMarkValue(GC* gc, Value* value){
    if(value->type != OBJ_VALUE) return;
    gcMarkObj(gc, value->val.obj);
}

void gcMarkObj(GC* gc, Obj* obj){
    if(obj == NULL) return;
    if(obj->marked) return;
    obj->marked = true;
    gcPushGray(gc, obj);
}

void gcMarkTable(GC* gc, Table* table){
    for(int i = 0; i < table->capacity; i++){
        Entry* entry = &table->entries[i];
        Obj* key = entry->key;
        gcMarkObj(gc, key);
        gcMarkValue(gc, &entry->value);
    }
}

void gcMarkChunk(GC* gc, Chunk* chunk){
    for(int i = 0; i < chunk->constants.count; i++){
        gcMarkValue(gc, &chunk->constants.values[i]);
    }
}

void gcMarkRoots(VM* vm){
    // mark call frames
    GC* gc = &vm->gc;
    for(int i = 1; i < vm->frame_count; i++){
        printf("marking call frame [%d]...", i);
        gcMarkObj(gc, vm->call_frames[i].fn);
        printf("finished\n");
    }

    // mark stack
    for(Value* sp = vm->stack; sp <= vm->stack_top; sp++)
        gcMarkValue(gc, sp);
    
    // mark global table
    gcMarkTable(gc, &vm->globals);
}

void gcSweep(ObjHeap* heap){
    Obj* obj = heap->objects;
    Obj* prev = NULL;

    #ifdef DEBUG_GC
    size_t before = heap->bytes_allocated;
    #endif

    while(obj != NULL){
        if(obj->marked){
            obj->marked = false;
            prev = obj;
            obj = obj->next;
        } else {
            Obj* unreachable = obj;
            obj = obj->next;
            if(prev != NULL)
                prev->next = obj;
            else 
                heap->objects = obj;
            
            #ifdef DEBUG_GC
            printf("collected %p\n", unreachable);
            #endif
            
            heap->bytes_allocated -= unreachable->size;
            freeObj(unreachable);
        }
    }
    
    heap->next_gc = heap->bytes_allocated * GC_HEAP_GROW_FACTOR;
    if(heap->next_gc < INIT_NEXT_GC) heap->next_gc = INIT_NEXT_GC;

    #ifdef DEBUG_GC
    printf("[before gc] %zu bytes allocated\n", before);
    printf("[after gc] %zu bytes remain, next gc at %zu\n", heap->bytes_allocated, heap->next_gc);
    #endif
}

void initGC(GC* gc){
    gc->gray_capacity = 0;
    gc->gray_count = 0;
    gc->gray_stack = NULL;
    gc->enabled = true;
}

void freeGC(GC* gc){
    gc->gray_capacity = 0;
    gc->gray_count = 0;
    free(gc->gray_stack);
    gc->gray_stack = NULL;
}

void gcBlackenObj(GC* gc, Obj* obj){
    if(obj) obj->vtable->blacken(gc, obj);
}

void tableRemoveWhite(Table* table){
    for(int i = 0; i < table->capacity; i++){
        Entry *entry = &table->entries[i];
        if(entry->key != NULL)
            if(!entry->key->obj.marked)
                tableDelete(table, entry->key);
    }
}

void gcCollect(VM* vm){
    if(vm == NULL) return;
    GC* gc = &vm->gc;

    if(!gc->enabled) return;
    gcMarkRoots(vm);
    
    while(gc->gray_count != 0){
        Obj* obj = gcPopGray(gc);
        gcBlackenObj(gc, obj);
    }

    tableRemoveWhite(vm->heap.strings);
    gcSweep(&vm->heap);
}

void gcPushGray(GC* gc, Obj* obj){
    if(gc->gray_count >= gc->gray_capacity){
        size_t old_capacity = gc->gray_capacity;
        gc->gray_capacity = growCapacity(old_capacity);
        gc->gray_stack = growArray(sizeof(Obj*), gc->gray_stack, old_capacity, gc->gray_capacity);
    }
    gc->gray_stack[gc->gray_count++] = obj;
}

Obj* gcPopGray(GC* gc){
    return gc->gray_stack[--gc->gray_count];
}
