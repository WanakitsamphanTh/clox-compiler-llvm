#include "compiler/resolve_scope.h"
#include "memory.h"
#include <string.h>

SymbolPool all_symbols = {.head = NULL};
ScopePool all_scopes = {.head = NULL};

void initResolver(ScopeResolver* resolver){
    resolver->depth = 0;
    resolver->global = newScope(NULL, 0);
    resolver->current = resolver->global;
    resolver->has_error = false;
    resolver->slot = 0;
}

void freeScopesAndSymbols(){
    freeScopes(&all_scopes);
    freeSymbols(&all_symbols);
}

Symbol* newSymbol(SymbolType type, const char* name, const size_t len, int depth, int slot){
    Symbol* sym = malloc(sizeof(Symbol) + len + 1);
    if(sym == NULL) return NULL;

    struct SymbolNode* node = malloc(sizeof(struct SymbolNode));
    if(node == NULL)  return NULL;

    sym->depth = depth;
    sym->slot = slot;
    sym->type = type;
    sym->length = len;
    memcpy(sym->name, name, len);
    sym->name[len] = '\0';

    node->symbol = sym;
    node->next = all_symbols.head;
    all_symbols.head = node;

    return sym;
}

Scope* newScope(Scope* parent, int depth){
    Scope* scope = malloc(sizeof(Scope));
    if(scope == NULL)  return NULL;

    struct ScopeNode* node = malloc(sizeof(struct ScopeNode));
    if(node == NULL)  return NULL;

    scope->capacity = 0;
    scope->symbol_count = 0;
    scope->locals = NULL;
    scope->parent = parent;
    scope->depth = depth;

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
    int i;
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

void beginScope(ScopeResolver* resolver){
    resolver->depth++;
    Scope* new = newScope(resolver->current, resolver->depth);
    if(new == NULL){
        RESOLVER_ERROR("New scope initialization failed\n");
        return;
    }
    resolver->current = new;
} 

void endScope(ScopeResolver* resolver){
    if(resolver->depth == 0){
        PARSER_ERROR("Attempted to retract from the global scope");
        return;
    }
    resolver->depth--;
    resolver->slot -= resolver->current->symbol_count;
    resolver->current = resolver->current->parent;
}

void scopeAddLocal(Scope* scope, Symbol* local){
    if(scope->symbol_count + 1 > scope->capacity){
        size_t old_capacity = scope->capacity;
        scope->capacity = growCapacity(old_capacity);
        scope->locals = growArray(sizeof(Symbol*), scope->locals, old_capacity, scope->capacity);
    }
    scope->locals[scope->symbol_count++] = local;
}

#define TERMINATE_RESOLVER_IF_ERROR() do { \
                                        if(resolver->has_error) { \
                                            success = false; \
                                            goto terminate_resolver; \
                                        } \
                                    } while(1)
#define RESOLVER_ERROR(...) do { \
                                snprintf(resolver->error_msg, 256, __VA_ARGS__); \
                                resolver->has_error = true; \
                            } while(1)

bool resolve(ScopeResolver* resolver, StmtList* stmts){
    int i = 0;
    bool success = true;
    
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
            beginScope(resolver);
            success = resolve(resolver, &stmt->body._block->stmt_list); TERMINATE_RESOLVER_IF_ERROR();
            endScope(resolver);
            break;

        // Declaration
        case VAR_DECL:
            success = resolveExpr(resolver, stmt->body._var_decl->init_expr); TERMINATE_RESOLVER_IF_ERROR();
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
                RESOLVER_ERROR("cannot allocate a new symbol for declaration at line %d\n", stmt->body._var_decl->name.line);
                TERMINATE_RESOLVER_IF_ERROR();
            }
            stmt->body._var_decl->symbol = symbol;
            scopeAddLocal(resolver->current, symbol);
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
        case ASSIGNMENT_EXPR:
            success = resolveExpr(resolver, expr->body._assign->val); TERMINATE_RESOLVER_IF_ERROR();
            Symbol* symbol = lookUpSymbol(resolver, expr->body._assign->var.start, expr->body._assign->var.length);
            if(symbol == NULL){
                RESOLVER_ERROR("Cannot resolve symbol at line %d", expr->body._assign->var.line);
                TERMINATE_RESOLVER_IF_ERROR();
            }            
            expr->body._assign->symbol = symbol;
            break;
        case VAR_EXPR:{
            Symbol* symbol = lookUpSymbol(resolver, expr->body._var->name.start, expr->body._var->name.length);
            if(symbol == NULL){
                RESOLVER_ERROR("Cannot resolve symbol at line %d", expr->body._var->name.line);
                TERMINATE_RESOLVER_IF_ERROR();
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
        case LITERAL_EXPR:
            return true;
    }

    terminate_resolver:
    return success;
}

Symbol* lookUpSymbol(ScopeResolver* resolver, const char* name, size_t length){
    Scope* scope = resolver->current;
    Symbol* symbol;
    while(scope != NULL){
        symbol = scopeLookUpSymbol(scope, name, length);
        if(symbol != NULL) return symbol;
        scope = scope->parent;
    }
    return NULL;
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


#undef RESOLVER_ERROR
#undef TERMINATE_RESOLVER_IF_ERROR