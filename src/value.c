#include "value.h"
#include "memory.h"
#include "vm.h"
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

Obj* AllocateObj(ObjType type, void (*destructor)(void*), size_t size){
    Obj* obj = malloc(size);
    obj->type = type;
    obj->destructor = destructor;

    obj->next = vm.objects;
    vm.objects = obj;

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

ObjString* makeObjString(const char* src, int length){
    uint8_t hash = hashString(src, length);
    ObjString* string = tableFindString(&vm.strings, src, length, hash);
    if(string == NULL) string = newObjString(src, length, hash);
    return string;
}

ObjString* newObjString(const char* src, size_t len, uint32_t hash){
    ObjString* str = AllocateObj(OBJ_STRING, NULL, sizeof(ObjString) + (len + 1));

    str->len = len;
    memcpy(str->str, src, len);
    str->str[len] = '\0';
    str->hash = hash; 

    tableSet(&vm.strings, str, VALUE_NIL);  // add new string to the string pool

    return str;
}

ObjString* concatObjString(const ObjString *s1, const ObjString *s2){
    size_t len = s1->len + s2->len;
    ObjString* str = AllocateObj(OBJ_STRING, NULL, sizeof(ObjString) + (len + 1));
    str->len = len;
    strcpy(str->str, s1->str);
    strcpy(str->str + s1->len, s2->str);
    str->str[str->len] = '\0';
    str->hash = 0;          /* hash is no need in runtime*/
    return str;
}

void valueToString(Value v, char* buffer){
    switch(v.type){
        case BOOL_VALUE: 
            sprintf(buffer, "%s", v.val.b? "true" : "false"); 
            break;
        case NUMBER_VALUE:
            sprintf(buffer, "%.8f", v.val.num); 
            break;
        case NIL_VALUE:
            sprintf(buffer, "nil"); 
            break;
        case OBJ_VALUE:
            switch(v.val.obj->type){
                case OBJ_STRING:{
                    sprintf(buffer, "%s", AS_CSTR(v));
                    break;
                }
                default:
                    sprintf(buffer, "[Object Instance]");
            }
    }
}

ObjString* valueToObjString(Value v){
    static char buffer[256];
    valueToString(v, buffer);
    return newObjString(buffer, strlen(buffer), 0);     /* no need for runtime string*/
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
    /*if(v1.type != v2.type) return false;
    if(v1.type == OBJ_VALUE) return v1.val.obj == v2.val.obj;
    if(v1.type == NUMBER_VALUE) return v1.val.num == v2.val.num;
    if(v1.type == BOOL_VALUE) return v1.val.b == v2.val.b;
    return false;*/
}