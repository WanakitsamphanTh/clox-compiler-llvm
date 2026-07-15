#include "obj.h"
#include "value.h"
#include "table.h"
#include "vm.h"
#include "stdio.h"

static void obj_array_destructor(Obj*);
static void obj_array_blacken(GC* gc, Obj*);

struct _Obj_Vtable obj_vtable = {.destructor = NULL, .blacken = obj_blacken};
struct _Obj_Vtable obj_array_vtable = {.destructor = &obj_array_destructor, .blacken = obj_array_blacken};
struct _Obj_Vtable obj_str_vtable = {.destructor = NULL, .blacken = obj_blacken};

Obj* AllocateObj(ObjHeap* heap, ObjType type, struct _Obj_Vtable* vtable, size_t size){
    heapCheck(heap);
    Obj* obj = malloc(size);
    if(obj == NULL){
        fprintf(stderr, "Cannot allocate new memory...\n");
        exit(1);
    }
    obj->type = type;
    obj->vtable = vtable;
    obj->size = size;

    obj->next = heap->objects;
    heap->objects = obj;
    heap->bytes_allocated += size;
    return obj;
}

uint32_t hashString(const char* key, int length){
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* makeObjString(ObjHeap* heap, const char* src, int length){
    uint32_t hash = hashString(src, length);
    ObjString* string = tableFindString(heap->strings, src, length, hash);
    if(string == NULL) string = newObjString(heap, src, length, hash);
    return string;
}

ObjString* newObjString(ObjHeap* heap, const char* src, size_t len, uint32_t hash){
    ObjString* str = AllocateObj(heap, OBJ_STRING, &obj_str_vtable, sizeof(ObjString) + (len + 1));
    str->len = len;
    memcpy(str->str, src, len);
    str->str[len] = '\0';
    str->hash = hash; 
    tableSet(heap->strings, str, VALUE_NIL);  // add new string to the string pool
    return str;
}

ObjString* concatObjString(ObjHeap* heap, const ObjString *s1, const ObjString *s2){
    size_t len = s1->len + s2->len;
    ObjString* str = AllocateObj(heap, OBJ_STRING, &obj_str_vtable, sizeof(ObjString) + (len + 1));
    str->len = len;
    strcpy(str->str, s1->str);
    strcpy(str->str + s1->len, s2->str);
    str->str[str->len] = '\0';
    str->hash = 0;          /* hash is no need in runtime*/
    return str;
}

ObjString* valueToObjString(ObjHeap* heap, Value v){
    static char buffer[256];
    memset(buffer,0,256);
    valueToString(v, buffer, 255);
    return newObjString(heap, buffer, strlen(buffer), 0);     /* no need for runtime string*/
}

bool isObjType(Value v, ObjType t){
    if(v.type != OBJ_VALUE) return false;
    return v.val.obj->type == t;
}

void freeObj(Obj* obj){
    if(obj){
        if(obj->vtable && obj->vtable->destructor)
            obj->vtable->destructor(obj);
        free(obj);
    }
}

ObjArray* makeObjArray(ObjHeap *heap, size_t len, Value* v_arr){
    ObjArray* arr = AllocateObj(heap, OBJ_ARRAY, &obj_array_vtable, sizeof(ObjArray) + sizeof(Value) * len);
    int i;
    if(v_arr != NULL)
        for(i = 0; i < len; i++)
            arr->elements[i] = v_arr[i];
    arr->len = len;
    return arr;
}

ObjArray* concatObjArray(ObjHeap* heap, const ObjArray* a, const ObjArray* b){
    ObjArray* arr = AllocateObj(heap, OBJ_ARRAY, &obj_array_vtable, sizeof(ObjArray) + sizeof(Value) * (a->len + b->len));
    int i, j;
    for(i = 0; i < a->len; i++)
        arr->elements[i] = a->elements[i];
    for(j = 0; j < b->len; j++)
        arr->elements[i+j] = b->elements[j];
    arr->len = a->len + b->len;
    return arr;
}

void initObjHeap(ObjHeap* heap){
    heap->objects = NULL;
    heap->bytes_allocated = 0;
    heap->next_gc = INIT_NEXT_GC;
    heap->strings = malloc(sizeof(Table));
    heap->owner = NULL;
    initTable(heap->strings);
}

void freeObjHeap(ObjHeap* heap){
    Obj* obj = heap->objects;
    Obj* next;
    freeTable(heap->strings);
    free(heap->strings);
    while(obj != NULL){
        next = obj->next;
        freeObj(obj);
        obj = next;
    }
}

void obj_blacken(GC* gc, Obj* obj){}

void obj_array_destructor(Obj* obj){
    return;
}

void obj_array_blacken(GC* gc, Obj* obj){
    ObjArray* array = obj;
    for(int i = 0; i < array->len; i++)
        gcMarkValue(gc, &array->elements[i]);
}

void heapCheck(ObjHeap* heap){
    if(heap->bytes_allocated > heap->next_gc && heap->owner != NULL) // run GC only when the heap is owned by a VM
        gcCollect(heap->owner);
}