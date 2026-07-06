#include "compiler/resolve_scope.h"
#include <string.h>

SymbolPool all_symbols = {.head = NULL};
ScopePool all_scopes = {.head = NULL};

void initResolver(ScopeResolver* resolver){
    resolver->depth = 0;
    resolver->global = newScope(NULL, 0);
    resolver->current = resolver->global;
}

void freeResolver(ScopeResolver* resolver){
    freeScopes(&all_scopes);
    freeSymbols(&all_symbols);
}

Symbol* newSymbol(SymbolType type, const char* name, const size_t len, int depth, int slot){
    Symbol* sym = malloc(sizeof(Symbol) + len + 1);
    struct SymbolNode* node = malloc(sizeof(struct SymbolNode));

    sym->depth = depth;
    sym->slot = slot;
    sym->type = type;
    sym->length = len;
    memcpy(sym->name, name, len);

    node->symbol = sym;
    node->next = all_symbols.head;
    all_symbols.head = node;

    return sym;
}

Scope* newScope(Scope* parent, int depth){
    Scope* scope = malloc(sizeof(Scope));
    struct ScopeNode* node = malloc(sizeof(struct ScopeNode));

    scope->capacity = 0;
    scope->symbol_count = 0;
    scope->symbols = NULL;

    node->scope = scope;
    node->next = all_scopes.head;
    all_scopes.head = node;
}

void freeScopes(ScopePool* scopes){
    struct ScopeNode* next = scopes->head;
    struct ScopeNode* node;
    while(next != NULL){
        node = next;
        next = node->next;
        freeScope(node->scope);
        free(node);
    }
}

void freeScope(Scope* scope){
    int i;
    free(scope->symbols);
    free(scope);
}

void freeSymbols(SymbolPool* symbols){
    struct SymbolNode* next = symbols->head;
    struct SymbolNode* node;
    while(next != NULL){
        node = next;
        next = node->next;
        free(node->symbol);
        free(node);
    }
}

void beginScope(ScopeResolver* resolver){
    resolver->depth++;
} 

void endScope(ScopeResolver* resolver){
    resolver->depth--;
}