#include "value.h"
#include "memory.h"
//#include "vm.h"
#include "function.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ValueArray newValueArray(){
    ValueArray v_array = {0, 0, NULL};
    return v_array;
}

void writeValueArray(ValueArray* array, Value value){
    int old_capacity;
    
    if(array->capacity < array->count + 1) {
        old_capacity = array->capacity;
        array->capacity = growCapacity(old_capacity);
        array->values = growArray(sizeof(Value), array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    freeArray(sizeof(Value), array->values, array->capacity);
    *array = newValueArray();
}

Obj* AllocateObj(ObjHeap* heap, ObjType type, void (*destructor)(void*), size_t size){
    Obj* obj = malloc(size);
    obj->type = type;
    obj->destructor = destructor;

    obj->next = heap->objects;
    heap->objects = obj;

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
    ObjString* str = AllocateObj(heap, OBJ_STRING, NULL, sizeof(ObjString) + (len + 1));

    str->len = len;
    memcpy(str->str, src, len);
    str->str[len] = '\0';
    str->hash = hash; 

    tableSet(heap->strings, str, VALUE_NIL);  // add new string to the string pool

    return str;
}

ObjString* concatObjString(ObjHeap* heap, const ObjString *s1, const ObjString *s2){
    size_t len = s1->len + s2->len;
    ObjString* str = AllocateObj(heap, OBJ_STRING, NULL, sizeof(ObjString) + (len + 1));
    str->len = len;
    strcpy(str->str, s1->str);
    strcpy(str->str + s1->len, s2->str);
    str->str[str->len] = '\0';
    str->hash = 0;          /* hash is no need in runtime*/
    return str;
}

/*int valueToString(Value v, char* buffer, int index, int max_length){
    buffer += index;
    switch(v.type){
        case BOOL_VALUE: 
            return snprintf(buffer, max_length, "%s", v.val.b? "true" : "false"); 
        case NUMBER_VALUE:
            return snprintf(buffer, max_length, "%.8f", v.val.num); 
        case NIL_VALUE:
            return snprintf(buffer, max_length, "nil"); 
        case OBJ_VALUE:
            switch(v.val.obj->type){
                case OBJ_STRING:{
                    return snprintf(buffer, max_length, "%s", AS_CSTR(v));
                }
                case OBJ_ARRAY:{
                    int i, length = 0;

                    snprintf(buffer, max_length, "{");
                    index++; 
                    length++;
                    max_length--;

                    ObjArray* arr = v.val.obj;

                    for(i = 0; i < arr->len; i++){
                        int required_char = valueToString(arr->elements[i], buffer, index, max_length); 
                        if(required_char > max_length){
                            length += snprintf(buffer, max_length, "...");
                        }
                        else length += required_char;
                        buffer += length;
                        max_length -= length;
                    }

                    sprintf(buffer+ length - ((length > 1)? 1 : 0), "}");
                    break;
                }
                default:
                    snprintf(buffer, max_length, "[Object Instance]");
            }
    }
}*/

int valueToString(Value v, char* buffer, int max_length){
    char* start = buffer;
    char* end = buffer + max_length;

    #define WRITE(...) \
        do { \
            int n = snprintf(buffer, end - buffer, __VA_ARGS__); \
            if (n < 0) return -1; \
            if (n >= end - buffer) return end - start; \
            buffer += n; \
        } while(0)

    switch(v.type){
        case BOOL_VALUE:
            WRITE("%s", v.val.b ? "true" : "false");
            break;
        case NUMBER_VALUE:
            WRITE("%.8f", v.val.num);
            break;
        case NIL_VALUE:
            WRITE("nil");
            break;

        case OBJ_VALUE:
            switch(v.val.obj->type){
                case OBJ_STRING:
                    WRITE("%s", AS_CSTR(v));
                    break;
                case OBJ_ARRAY: {
                    ObjArray* arr = (ObjArray*)v.val.obj;
                    WRITE("{");
                    for(int i = 0; i < arr->len; i++){
                        if(i > 0) WRITE(",");
                        int remaining_before = end - buffer;
                        int used = valueToString(arr->elements[i], buffer, remaining_before);
                        if(used < 0) return -1;

                        // if element didn't fully fit, stop and ensure closure
                        if(used >= remaining_before){
                            WRITE("...");
                            break;
                        }
                        buffer += used;
                    }

                    // always try to write closing brace
                    if(buffer < end){
                        WRITE("}");
                    } else {
                        // fallback: ensure logical correctness even if truncated
                        buffer = end;
                    }
                    break;
                }
                case OBJ_CALLABLE:
                    WRITE("<Callable Object: %s>", ((ObjCallable*) v.val.obj)->name->str);
                    break;
                default:
                    WRITE("<Object Instance>");
            }
            break;
    }

    #undef WRITE

    return buffer - start;
}

ObjString* valueToObjString(ObjHeap* heap, Value v){
    static char buffer[256];
    memset(buffer,0,256);
    valueToString(v, buffer, 255);
    return newObjString(heap, buffer, strlen(buffer), 0);     /* no need for runtime string*/
}

char* decodeString(char* str){
    char *c1, *c2;
    c1 = c2 = str;
    while(*c2){
        if(*c2 == '\\'){
            c2++;
            if(*c2 == '\0') break;
            switch(*c2){
                case '\\': *(c1++) = '\\'; break;
                case 'n': *(c1++) = '\n'; break;
                case 't': *(c1++) = '\t'; break;
                case '"': *(c1++) = '"'; break;
                case '0': *(c1++) = '\0'; break;
                default:
                    *(c1++) = '\\';
                    *c1 = *c2; 
            }
        }
        else {
            *(c1++) = *c2;
        }
        c2++;
    }
    *c1 = '\0';
    int len = c1 - str;
    return realloc(str, len + 1);
}

void encodeString(char* dist, const char* str){
    char *c1 = dist;
    const char *c2 = str;
    while(*c2){
        switch(*c2){
            case '\n': *c1 = '\\'; *(++c1) = 'n'; break;
            case '\t': *c1 = '\\'; *(++c1) = 't'; break;
            case '"': *c1 = '\\'; *(++c1) = '"'; break;
            case '\\': *c1 = '\\'; *(++c1) = '\\'; break;
            default:
                *c1 = *c2;
        }
        c1++;
        c2++;
    }
    *c1 = '\0';
}

bool isObjType(Value v, ObjType t){
    if(v.type != OBJ_VALUE) return false;
    return v.val.obj->type == t;
}

void freeObj(Obj* obj){
    if(obj){
        if(obj->destructor)
            obj->destructor(obj);
        free(obj);
    }
}

bool compareValue(Value v1, Value v2){
    if(v1.type != v2.type) return false;
    if(v1.type == OBJ_VALUE) return v1.val.obj == v2.val.obj;
    if(v1.type == NUMBER_VALUE) return v1.val.num == v2.val.num;
    if(v1.type == BOOL_VALUE) return v1.val.b == v2.val.b;
    if(v1.type == NIL_VALUE) return true;
    return false;
}

ObjArray* makeObjArray(ObjHeap *heap, size_t len, Value* v_arr){
    ObjArray* arr = AllocateObj(heap, OBJ_ARRAY, NULL, sizeof(ObjArray) + sizeof(Value) * len);
    int i;
    for(i = 0; i < len; i++)
        arr->elements[i] = v_arr[i];
    arr->len = len;
    return arr;
}

ObjArray* concatObjArray(ObjHeap* heap, const ObjArray* a, const ObjArray* b){
    ObjArray* arr = AllocateObj(heap, OBJ_ARRAY, NULL, sizeof(ObjArray) + sizeof(Value) * (a->len + b->len));
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
    heap->strings = malloc(sizeof(Table));
    initTable(heap->strings);
}

void transferObjHeap(ObjHeap* dist, ObjHeap* src){
    dist->objects = src;
    src->objects = NULL;
    dist->strings = src->strings;
    dist->strings = NULL;
    free(src);
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