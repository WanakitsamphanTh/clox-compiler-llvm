#ifndef RESOLVE_SCOPE_H
#define RESOLVE_SCOPE_H
#include "common.h"
#include "compiler/token.h"
#include "natfn.h"

#define LOCAL_MAX 256

typedef struct _Stmt Stmt;
typedef struct _expression Expr;
typedef struct _StmtList StmtList;

typedef enum {
    SYM_GLOB,
    SYM_LOC,
    SYM_UVAL
} SymbolType;

typedef enum {
    SCRIPT_TYPE,
    FUNCTION_TYPE
} ScriptType;

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
    ScriptType script_type;
    int depth;
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
    int slot;

    char error_msg[256];
    bool has_error;
} ScopeResolver;


void initResolver(ScopeResolver*);
void freeScopesAndSymbols();
void beginScope(ScopeResolver*,ScriptType);
void endScope(ScopeResolver*);
bool scopeAddLocal(Scope*, Symbol*);
Symbol* lookUpSymbol(ScopeResolver*, const char*, size_t);
Symbol* scopeLookUpSymbol(Scope*, const char*, size_t);
static void resolverAddNatives(void*, const char*, int, NativeFn);

Symbol* newSymbol(SymbolType, const char*, const size_t, int, int);
Scope* newScope(Scope*, int, ScriptType);

void freeScopes(ScopePool*);
void freeSymbols(SymbolPool*);

bool resolve(ScopeResolver*, StmtList*);
static bool resolveStmt(ScopeResolver*, Stmt*);
static bool resolveExpr(ScopeResolver*, Expr*);

extern SymbolPool all_symbols;
extern ScopePool all_scopes;

#endif