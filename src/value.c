#include "value.h"
#include "memory.h"
#include <stddef.h>
#include <stdio.h>

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
        case STR_VALUE:
            sprintf(buffer, "%s", v.val.str);
    }
}