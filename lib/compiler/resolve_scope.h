#ifndef RESOLVE_SCOPE_H
#define RESOLVE_SCOPE_H
#include "common.h"
#include "compiler/token.h"
#include "compiler/statement.h"

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
    Symbol **locals;
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

    char error_msg[256];
    bool has_error;
} ScopeResolver;


void initResolver(ScopeResolver*);
void freeScopesAndSymbols();
void beginScope(ScopeResolver*);
void endScope(ScopeResolver*);
void scopeAddLocal(Scope*, Symbol*);
Symbol* lookUpSymbol(ScopeResolver*, const char*, size_t);
Symbol* scopeLookUpSymbol(Scope*, const char*, size_t);

Symbol* newSymbol(SymbolType, const char*, const size_t, int, int);
Scope* newScope(Scope*, int);

void freeScopes(ScopePool*);
void freeSymbols(SymbolPool*);

bool resolve(ScopeResolver*, StmtList*);
static bool resolveStmt(ScopeResolver*, Stmt*);
static bool resolveExpr(ScopeResolver*, Expr*);

extern SymbolPool all_symbols;
extern ScopePool all_scopes;

#endif