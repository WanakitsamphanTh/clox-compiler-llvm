#ifndef RESOLVE_SCOPE_H
#define RESOLVE_SCOPE_H
#include "common.h"
#include "compiler/token.h"

#define LOCAL_MAX 256

typedef enum {
    SYM_GLOB,
    SYM_LOC,
    SYM_UVAL
} SymbolType;

typedef struct {
    int depth;
    int slot;
    size_t length;
    SymbolType type;
    char name[];
} Symbol;

typedef struct _Scope {
    struct _Scope* parent;
    Symbol **symbols;
    size_t symbol_count;
    size_t capacity;
} Scope;

typedef struct {
    struct ScopeNode {
        Scope *scope;
        struct ScopeNode *next;
    }* head;
} ScopePool; // to release post-compilation

typedef struct {
    struct SymbolNode {
        Symbol *symbol;
        struct SymbolNode *next;
    }* head;
} SymbolPool; // to release post-compilation

typedef struct {
    Scope *global;
    Scope *current;
    int depth;
} ScopeResolver;


void initResolver(ScopeResolver*);
void freeResolver(ScopeResolver*);
void beginScope(ScopeResolver*);
void endScope(ScopeResolver*);
void scopeAddSymbol(Scope*, Symbol*);

Symbol* newSymbol(SymbolType, const char*, const size_t, int, int);
Scope* newScope(Scope*, int);

void freeScopes(ScopePool*);
void freeSymbols(SymbolPool*);

extern SymbolPool all_symbols;
extern ScopePool all_scopes;

#endif