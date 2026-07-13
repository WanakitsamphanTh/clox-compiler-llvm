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
    BLOCK_TYPE,
    FUNCTION_TYPE
} ScopeType;

typedef struct {
    int depth;
    int slot;
    size_t length;
    SymbolType type;
    bool captured;
    char name[];
} Symbol;

typedef struct _Scope {
    struct _Scope* parent;
    Symbol **locals;
    size_t symbol_count;
    size_t capacity;
    int depth;
    ScopeType type;
    Stmt* binding;
    Stmt* owner_function;
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
    Symbol* symbol;
    Scope* scope;
    bool crossed_fn_boundary;
} SymbolLookupResult;

typedef enum {
    UVAL_LOC,
    UVAL_UVAL
} UpValueType;

typedef struct {
    UpValueType type;
    int index;
} UpValue;

typedef struct _FnInfo {
    Scope* scope;
    int local_count;

    struct _FnInfo* parent;

    UpValue* upvalues;
    int upvalue_count;
    int upvalue_capacity;
} FnInfo;

typedef struct {
    struct _FnNode {
        FnInfo* fn_info;
        struct _FnNode* next;
    } *head;
} FnPool;

typedef struct {
    Scope *global;
    Scope *current;
    FnInfo *current_fn;

    int depth;
    int slot;

    char error_msg[256];
    bool has_error;
} ScopeResolver;

void initResolver(ScopeResolver*);
void freeScopesAndSymbols();
void beginScope(ScopeResolver*,ScopeType,Stmt*,FnInfo*);
void endScope(ScopeResolver*);
bool scopeAddLocal(Scope*, Symbol*);
SymbolLookupResult lookUpSymbol(ScopeResolver*, const char*, size_t);
Symbol* scopeLookUpSymbol(Scope*, const char*, size_t);
Symbol* functionLookUpSymbol(Scope*, Scope*, const char*, size_t);
static void resolverAddNatives(void*, const char*, int, NativeFn);

Symbol* newSymbol(SymbolType, const char*, const size_t, int, int);
Scope* newScope(Scope*, int, ScopeType, Stmt*, FnInfo*);
FnInfo* newFnInfo();

void freeScopes(ScopePool*);
void freeSymbols(SymbolPool*);
void freeFnInfos(FnPool*);

int fnInfoAddUpVal(FnInfo*, UpValue);

bool resolve(ScopeResolver*, StmtList*);
static bool resolveStmt(ScopeResolver*, Stmt*);
static bool resolveExpr(ScopeResolver*, Expr*);
static Symbol* resolveUpValue(ScopeResolver*, Symbol*, Scope*,FnInfo*);

extern SymbolPool all_symbols;
extern ScopePool all_scopes;
extern FnPool all_fn;

#endif