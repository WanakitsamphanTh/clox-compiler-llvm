#include "table.h"
#include "memory.h"
#include "value.h"
#include "obj.h"
#include <assert.h>
#include <string.h>

void initTable(Table* table){
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table){
    freeArray(sizeof(Entry), table->entries, table->capacity);
    initTable(table);
}


static void adjustCapacity(Table* table, int capacity) {
    int old_capacity = table->capacity;

    /* create new entries*/
    Entry* entries = reallocate(NULL, 0, sizeof(Entry) * capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = VALUE_NIL;
    }

    /* move entries*/
    int i;
    table->count = 0;
    for(i = 0; i < old_capacity; i++){
        Entry* entry = table->entries + i;
        if(entry->key == NULL) continue;
        
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry-> value;
        table->count++;
    }

    /* delete old entries */
    freeArray(sizeof(Entry), table->entries, old_capacity);
    //free(table->entries);

    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table* table, const ObjString* k, Value v){
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = growCapacity(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, k);
    bool is_new_key = entry->key == NULL;
    if(is_new_key && IS_NIL(entry->value)) table->count++;

    entry->key = k;
    entry->value = v;
    return is_new_key;
}

bool tableGet(Table* table, const ObjString* k, Value* vp){
    if(table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, k);
    if(entry->key == NULL) return false;
    *vp = entry->value;
    return true;
}

bool tableDelete(Table* table, const ObjString* k){
    if(table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->capacity, k);
    if(entry->key == NULL) return false;
    entry->key = NULL;
    entry->value = VALUE_BOOL(true);
    return true;
}

Entry* findEntry(Entry* entries, int capacity, const ObjString* k){
    assert(capacity > 0);
    uint32_t index = k->hash % capacity;
    Entry* tombstone = NULL;
    while(1){
        Entry* entry = entries + index;

        if(entry->key == NULL){
            if(IS_NIL(entry->value)) 
                return (tombstone != NULL)? tombstone : entry;
            else if(tombstone == NULL) 
                tombstone = entry;
        } else if(entry->key == k)
            return entry;

        index = (index + 1) % capacity;
    }
}

ObjString* tableFindString(Table* table, const char* k, size_t len, uint32_t hash){
    if(table->count == 0 || table->capacity == 0) return NULL;

    uint32_t index = hash % table->capacity;
    while(1){
        Entry* entry = &table->entries[index];
        if(entry->key == NULL){
            if(IS_NIL(entry->value)) return NULL;   /* non-tombstone bucket*/
        } else if(entry->key->len == len && entry->key->hash == hash && memcmp(entry->key->str, k, len) == 0)
            return entry->key;

        index = (index + 1) % table->capacity;
    }
}

void copyTable(Table* dist, Table* src){
    for(int i = 0; i < src->capacity; i++){
        Entry* entry = src->entries + i;
        if(entry->key) tableSet(dist, entry->key, entry->value);
    }
}