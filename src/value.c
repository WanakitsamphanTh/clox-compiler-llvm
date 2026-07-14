#include "value.h"
#include "memory.h"
#include "obj.h"
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

bool compareValue(Value v1, Value v2){
    if(v1.type != v2.type) return false;
    if(v1.type == OBJ_VALUE) return v1.val.obj == v2.val.obj;
    if(v1.type == NUMBER_VALUE) return v1.val.num == v2.val.num;
    if(v1.type == BOOL_VALUE) return v1.val.b == v2.val.b;
    if(v1.type == NIL_VALUE) return true;
    return false;
}
