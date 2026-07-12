#ifndef TABLE_H
#define TABLE_H
#include "common.h"
#include "value.h"

typedef struct _ObjString ObjString;

typedef struct {
    const ObjString* key;
    Value value;
} Entry;

typedef struct _Table {
    int capacity;
    int count;
    Entry* entries;
} Table;

void initTable(Table*);
void freeTable(Table*);
bool tableSet(Table*, const ObjString*, Value);
bool tableGet(Table*, const ObjString*, Value*);
bool tableDelete(Table*, const ObjString*);
ObjString* tableFindString(Table*, const char*, size_t , uint32_t );
Entry* findEntry(Entry*, int, const ObjString*);

void copyTable(Table*, Table*);

#define TABLE_MAX_LOAD 0.8

#endif