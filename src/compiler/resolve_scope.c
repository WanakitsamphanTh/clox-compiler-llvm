#include "compiler/resolve_scope.h"
#include "compiler/statement.h"
#include "compiler/expression.h"
#include "memory.h"
#include <string.h>

#define TERMINATE_RESOLVER_IF_ERROR() do { \
                                        if(resolver->has_error) { \
                                            success = false; \
                                            goto terminate_resolver; \
                                        } \
                                    } while(0)
#define RESOLVER_ERROR(...) do { \
                                snprintf(resolver->error_msg, 256, __VA_ARGS__); \
                                resolver->has_error = true; \
                            } while(0)

SymbolPool all_symbols = {.head = NULL};
ScopePool all_scopes = {.head = NULL};
FnPool all_fn = {.head = NULL};

void initResolver(ScopeResolver* resolver){
    resolver->depth = 0;
    resolver->current_fn = NULL;
    resolver->global = newScope(NULL, 0, SCRIPT_TYPE, NULL, NULL);
    resolver->current = resolver->global;
    resolver->has_error = false;
    resolver->slot = 0;
    defineNativeFunctions(resolver, &resolverAddNatives);
}

void freeScopesAndSymbols(){
    //printf("freeing scopes...\n");
    freeScopes(&all_scopes);
    //printf("scopes freed\n");
    //printf("freeing symbols...\n");
    freeSymbols(&all_symbols);
    //printf("symbols freed\n");
    freeFnInfos(&all_fn);
    //printf("FnInfo's freed\n");
}

Symbol* newSymbol(SymbolType type, const char* name, const size_t len, int depth, int slot){
    Symbol* sym = malloc(sizeof(Symbol) + len + 1);
    if(sym == NULL) return NULL;

    struct SymbolNode* node = malloc(sizeof(struct SymbolNode));
    if(node == NULL)  {
        return NULL;
    }

    sym->depth = depth;
    sym->slot = slot;
    sym->type = type;
    sym->length = len;
    sym->captured = false;
    memcpy(sym->name, name, len);
    sym->name[len] = '\0';

    node->symbol = sym;
    node->next = all_symbols.head;
    all_symbols.head = node;

    return sym;
}

Scope* newScope(Scope* parent, int depth, ScopeType type, Stmt* binding, FnInfo* owner_function){
    Scope* scope = malloc(sizeof(Scope));
    if(scope == NULL)  return NULL;

    struct ScopeNode* node = malloc(sizeof(struct ScopeNode));
    if(node == NULL)  {
        return NULL;
    }

    scope->capacity = 0;
    scope->symbol_count = 0;
    scope->locals = NULL;
    scope->parent = parent;
    scope->depth = depth;
    scope->type = type;
    scope->binding = binding;
    scope->owner_function = owner_function;
    if(type == FUNCTION_TYPE) owner_function->scope = scope;

    node->scope = scope;
    node->next = all_scopes.head;
    all_scopes.head = node;

    return scope;
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
    free(scope->locals);
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

void beginScope(ScopeResolver* resolver, ScopeType type, Stmt* binding, FnInfo* owner_function){
    resolver->depth++;
    Scope* new = newScope(resolver->current, resolver->depth, type, binding, owner_function);
    if(new == NULL){
        RESOLVER_ERROR("New scope initialization failed\n");
        return;
    }
    resolver->current = new;
    if(type == FUNCTION_TYPE) { // begin a new function
        owner_function->parent = resolver->current_fn;
        resolver->current_fn = owner_function;
    }
} 

void endScope(ScopeResolver* resolver){
    if(resolver->depth == 0){
        RESOLVER_ERROR("Attempted to retract from the global scope");
        return;
    }
    resolver->depth--;
    resolver->slot -= resolver->current->symbol_count;
    if(resolver->current->type == FUNCTION_TYPE) 
        resolver->current_fn =  resolver->current_fn->parent;
    resolver->current = resolver->current->parent;
}

bool scopeAddLocal(Scope* scope, Symbol* local){
    if(scope->symbol_count + 1 > scope->capacity){
        size_t old_capacity = scope->capacity;
        scope->capacity = growCapacity(old_capacity);
        scope->locals = growArray(sizeof(Symbol*), scope->locals, old_capacity, scope->capacity);
        if(scope->locals == NULL) {
            return false;
        }
    }
    scope->locals[scope->symbol_count++] = local;
    return true;
}

FnInfo* newFnInfo(){
    FnInfo* fn = malloc(sizeof(FnInfo));
    fn->upvalues = NULL;
    fn->upvalue_count = 0;
    fn->upvalue_capacity = 0;
    fn->parent = NULL;
    fn->scope = NULL;
    fn->local_count = 0;

    struct _FnNode* node = malloc(sizeof(struct _FnNode));
    node->fn_info = fn;
    node->next = all_fn.head;
    all_fn.head = node;
    return fn;
}

void freeFnInfos(FnPool* pool){
    struct _FnNode *node, *next;
    node = pool->head;
    while(node != NULL){
        next = node->next;
        free(node->fn_info->upvalues);
        free(node->fn_info);
        free(node);
        node = next;
    }
}

int fnInfoAddUpVal(FnInfo* fn, UpValue uv){
    for(int i = 0; i < fn->upvalue_count; i++){
        UpValue* existing = fn->upvalues + i;
        if(existing->index == uv.index && existing->type == uv.type)
            return i;
    }
    if(fn->upvalue_count + 1 > fn->upvalue_capacity){
        int old_capacity = fn->upvalue_capacity;
        fn->upvalue_capacity = growCapacity(fn->upvalue_capacity);
        fn->upvalues = growArray(sizeof(UpValue), fn->upvalues, old_capacity, fn->upvalue_capacity);
    }
    fn->upvalues[fn->upvalue_count] = uv;
    return fn->upvalue_count++;
}

bool resolve(ScopeResolver* resolver, StmtList* stmts){
    int i = 0;
    bool success = true;
    
    // scan for declarations
    for(i = 0; i < stmts->count; i++){
        Stmt* stmt = stmts->stmt[i];
        Symbol* symbol;
        ScopeType type;
        int slot;
        if(stmt->type == VAR_DECL){
            if(resolver->depth == 0){
                type = SYM_GLOB;
                slot = -1;
            }
            else{
                type = SYM_LOC;
                slot = resolver->slot++;
            }
            if(scopeLookUpSymbol(resolver->current, stmt->body._var_decl->name.start, stmt->body._var_decl->name.length) != NULL){
                RESOLVER_ERROR("Variables with the same name cannot be re-declared in the same scope\n");
                TERMINATE_RESOLVER_IF_ERROR();
            }
            symbol = newSymbol(type, stmt->body._var_decl->name.start, stmt->body._var_decl->name.length, resolver->depth, slot);
            if(symbol == NULL){
                RESOLVER_ERROR("cannot allocate a new symbol for declaration at line %d\n", stmt->body._var_decl->name.line + 1);
                TERMINATE_RESOLVER_IF_ERROR();
            }
            stmt->body._var_decl->symbol = symbol;
            if(!scopeAddLocal(resolver->current, symbol)){
                RESOLVER_ERROR("cannot add a new variable to the current scope\n");
                TERMINATE_RESOLVER_IF_ERROR();
            }
        }
        else if(stmt->type == FN_DECL){
            if(resolver->depth == 0){
                type = SYM_GLOB;
                slot = -1;
            } else {
                type = SYM_LOC;
                slot = resolver->slot++;
            }

            if(scopeLookUpSymbol(resolver->current, stmt->body._fn_decl->name.start, stmt->body._fn_decl->name.length) != NULL){
                RESOLVER_ERROR("Cannot define functions with the same name\n");
                TERMINATE_RESOLVER_IF_ERROR();
            }

            symbol = newSymbol(type, stmt->body._fn_decl->name.start, stmt->body._fn_decl->name.length, resolver->depth, slot);
            stmt->body._fn_decl->symbol = symbol;
            if(!scopeAddLocal(resolver->current, symbol)){
                RESOLVER_ERROR("cannot add a new function to the current scope\n");
                TERMINATE_RESOLVER_IF_ERROR();
            }
        }
    }

    // resolve scopes
    for(i = 0; i < stmts->count; i++){
        success = resolveStmt(resolver, stmts->stmt[i]);
        TERMINATE_RESOLVER_IF_ERROR();
    }
    
    terminate_resolver:
    return success;
}

bool resolveStmt(ScopeResolver* resolver, Stmt*stmt){
    if(stmt == NULL) return true;
    if(resolver->has_error) return false;
    bool success = true;

    Symbol* symbol;
    SymbolType type;
    int slot;

    switch(stmt->type){
        case PRINT_STMT: 
            success = resolveExpr(resolver, stmt->body._print->expr); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case EXPR_STMT: 
            success = resolveExpr(resolver, stmt->body._expr->expr); TERMINATE_RESOLVER_IF_ERROR();
            break;

        // Flow control statements
        case IF_STMT: 
            success = resolveExpr(resolver, stmt->body._if->condition); TERMINATE_RESOLVER_IF_ERROR();
            success = resolveStmt(resolver, stmt->body._if->then_branch); TERMINATE_RESOLVER_IF_ERROR();
            success = resolveStmt(resolver, stmt->body._if->else_branch); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case WHILE_STMT: 
            success = resolveExpr(resolver, stmt->body._while->condition); TERMINATE_RESOLVER_IF_ERROR();
            success = resolveStmt(resolver, stmt->body._while->body); TERMINATE_RESOLVER_IF_ERROR();
            success = resolveStmt(resolver, stmt->body._while->increment); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case BREAK_STMT: 
        case SKIP_STMT: 
            return true;
        
        case BLOCK_STMT: 
            beginScope(resolver, BLOCK_TYPE, stmt, resolver->current->owner_function);
            success = resolve(resolver, &stmt->body._block->stmt_list); TERMINATE_RESOLVER_IF_ERROR();
            stmt->body._block->scope = resolver->current;
            endScope(resolver);
            break;

        // Declaration
        case VAR_DECL:
            success = resolveExpr(resolver, stmt->body._var_decl->init_expr); TERMINATE_RESOLVER_IF_ERROR();
            break;

        case FN_DECL:{
            int saved_slot = resolver->slot;
            FnInfo* fn_info = newFnInfo();
            stmt->body._fn_decl->info = fn_info;
            beginScope(resolver, FUNCTION_TYPE, stmt, fn_info);
            resolver->slot = 0;
        
            for(int i = 0; i < stmt->body._fn_decl->arity; i++){
                slot = resolver->slot++;
                symbol = newSymbol(SYM_LOC, stmt->body._fn_decl->args.names[i].start, stmt->body._fn_decl->args.names[i].length, resolver->depth, slot);
                stmt->body._fn_decl->args.symbols[i] = symbol;
                scopeAddLocal(resolver->current, symbol);
            }

            success = resolveStmt(resolver, stmt->body._fn_decl->body);
            TERMINATE_RESOLVER_IF_ERROR();
            fn_info->local_count = resolver->slot;
            endScope(resolver);
            resolver->slot = saved_slot;
            break;
        }
        
        case RETURN_STMT:
            if(resolver->current->owner_function == NULL) {
                RESOLVER_ERROR("return must be inside a function\n");
                TERMINATE_RESOLVER_IF_ERROR();
            }
            resolveExpr(resolver, stmt->body._ret->ret_val);
            break;
    }

    terminate_resolver:
    return success;
}

bool resolveExpr(ScopeResolver* resolver, Expr* expr){
    if(expr == NULL) return true;
    if(resolver->has_error) return false;
    bool success = true;

    switch(expr->type){
        case UNARY_EXPR:
            success = resolveExpr(resolver, expr->body._unary->right); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case BINARY_EXPR:
            success = resolveExpr(resolver, expr->body._bin->right); TERMINATE_RESOLVER_IF_ERROR();
            success = resolveExpr(resolver, expr->body._bin->left); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case ASSIGNMENT_EXPR:{
            success = resolveExpr(resolver, expr->body._assign->val); TERMINATE_RESOLVER_IF_ERROR();
            
            SymbolLookupResult result = lookUpSymbol(resolver, expr->body._assign->var.start, expr->body._assign->var.length);
            Symbol* symbol = result.symbol;

            /*if(symbol == NULL){
                RESOLVER_ERROR("Cannot resolve symbol at line %d", expr->body._assign->var.line);
                TERMINATE_RESOLVER_IF_ERROR();
            }            
            expr->body._assign->symbol = symbol;*/
            if(symbol == NULL){  
                RESOLVER_ERROR("Cannot resolve symbol at line %d\n", expr->body._assign->var.line + 1);
                TERMINATE_RESOLVER_IF_ERROR();
            }
            
            if(symbol->type != SYM_GLOB && result.crossed_fn_boundary){
                symbol = resolveUpValue(resolver, symbol, resolver->current_fn);
                if(symbol == NULL){
                    RESOLVER_ERROR("Cannot resolve upvalue symbol at line %d\n", expr->body._assign->var.line + 1);
                    TERMINATE_RESOLVER_IF_ERROR();
                }
            }

            expr->body._assign->symbol = symbol;
            break;
        }
        case VAR_EXPR:{
            SymbolLookupResult result = lookUpSymbol(resolver, expr->body._var->name.start, expr->body._var->name.length);
            Symbol* symbol = result.symbol;

            if(symbol == NULL){  
                RESOLVER_ERROR("Cannot resolve symbol at line %d\n", expr->body._var->name.line + 1);
                TERMINATE_RESOLVER_IF_ERROR();
            }
            
            if(symbol->type != SYM_GLOB && result.crossed_fn_boundary){
                symbol = resolveUpValue(resolver, symbol, resolver->current_fn);
                if(symbol == NULL){
                    RESOLVER_ERROR("Cannot resolve upvalue symbol at line %d\n", expr->body._var->name.line + 1);
                    TERMINATE_RESOLVER_IF_ERROR();
                }
            }

            expr->body._var->symbol = symbol;
            break;
        }
        case GROUP_EXPR:
            success = resolveExpr(resolver, expr->body._group->expr); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case ARR_EXPR:{
            int i;
            for(i = 0; i < expr->body._arr->count; i++){
                success = resolveExpr(resolver, expr->body._arr->elements[i]); 
                TERMINATE_RESOLVER_IF_ERROR();
            }
            break;
        }
        case CALL_EXPR: {
            int i;
            for(i = 0; i < expr->body._call->argc; i++){
                success = resolveExpr(resolver, expr->body._call->argv.list[i]); 
                TERMINATE_RESOLVER_IF_ERROR();
            }
            success = resolveExpr(resolver, expr->body._call->callee); TERMINATE_RESOLVER_IF_ERROR(); 
            break;
        }
        case INDEX_EXPR: 
            resolveExpr(resolver, expr->body._index->index); TERMINATE_RESOLVER_IF_ERROR();
            resolveExpr(resolver, expr->body._index->var); TERMINATE_RESOLVER_IF_ERROR();
            break;
        case LITERAL_EXPR:
            return true;
    }

    terminate_resolver:
    return success;
}

Symbol* resolveUpValue(ScopeResolver* resolver, Symbol* symbol, FnInfo* current){
    FnInfo* parent = current->parent;
    int index;
    if(parent == NULL) return NULL; // first layer function (defined in global) has no upvalue
    
    Symbol* parent_symbol = functionLookUpSymbol(parent->scope, symbol->name, symbol->length);
    
    if(parent_symbol != NULL) {
        // parent owns the local variable
        int r = fnInfoAddUpVal(current, (UpValue){.type = UVAL_LOC, .index = parent_symbol->slot});
        index = r; 
        parent_symbol->captured = true;
    }
    else { 
        // parent doesnt own the local variable
        Symbol* parent_uval = resolveUpValue(resolver, symbol, parent); // resolve first in parent
        if(parent_uval == NULL) return NULL;
        index = fnInfoAddUpVal(current,(UpValue){.type = UVAL_UVAL, .index = parent_uval->slot});
    }
    return newSymbol(SYM_UVAL, symbol->name, symbol->length, current->scope->depth, index);
}

SymbolLookupResult lookUpSymbol(ScopeResolver* resolver, const char* name, size_t length){
    Scope* scope = resolver->current;
    Symbol* symbol = NULL;
    bool crossed_fn_boundary = false;
    while(scope != NULL){
        symbol = scopeLookUpSymbol(scope, name, length);
        if(symbol != NULL) {
            return (SymbolLookupResult){.symbol = symbol, .scope = scope, .crossed_fn_boundary = crossed_fn_boundary};
        }
        if(scope->type == FUNCTION_TYPE)
            crossed_fn_boundary = true;
        scope = scope->parent;
    }
    return (SymbolLookupResult){NULL,NULL,false};
}

Symbol* scopeLookUpSymbol(Scope* scope, const char* name, size_t length){
    int i;
    Symbol* symbol;
    for(i = scope->symbol_count - 1; i >= 0; i--){
        symbol = scope->locals[i];
        if(symbol->length == length && memcmp(symbol->name, name, length) == 0)
            return symbol;
    }
    return NULL;
}

Symbol* functionLookUpSymbol(Scope* fn_scope, const char* name, size_t length){
    Scope* current = fn_scope;
    while(current != NULL){
        Symbol* parent_symbol = scopeLookUpSymbol(current, name, length);
        if(parent_symbol != NULL) return parent_symbol;
        if(current->type == FUNCTION_TYPE) break;
        current = current->parent;
    }
    return NULL;
}

void resolverAddNatives(void* resolver_ptr, const char* name, int _arity, NativeFn _fn){
    ScopeResolver *resolver = resolver_ptr;
    size_t len = strlen(name);
    Symbol* symbol = newSymbol(SYM_GLOB, name, len, 0, -1);
    scopeAddLocal(resolver->global, symbol);
}

#undef RESOLVER_ERROR
#undef TERMINATE_RESOLVER_IF_ERROR