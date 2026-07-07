#include "compiler/statement.h"
#include <stdlib.h>

Stmt* newStmt(StmtType type){
    Stmt *stmt = malloc(sizeof(Stmt));
    if(stmt == NULL) return NULL;
    stmt->type = type;
    switch(type){
        case PRINT_STMT:
            stmt->body._print = malloc(sizeof(PrintStmt)); 
            stmt->body._print->expr = NULL;
            break;
        case EXPR_STMT:
            stmt->body._expr = malloc(sizeof(ExprStmt)); 
            stmt->body._expr->expr = NULL;
            break;
        case IF_STMT:
            stmt->body._if = malloc(sizeof(IfStmt));
            stmt->body._if->condition = NULL;
            stmt->body._if->then_branch = stmt->body._if->else_branch = NULL;
            break;
        case WHILE_STMT:
            stmt->body._while = malloc(sizeof(WhileStmt)); 
            stmt->body._while->body = stmt->body._while->increment = NULL;
            stmt->body._while->condition  = NULL;
            break;
        case BREAK_STMT:
        case SKIP_STMT:
            break;
        case BLOCK_STMT:
            stmt->body._block = malloc(sizeof(BlockStmt)); 
            stmt->body._block->stmt_list = newStmtList();
            stmt->body._block->scope = NULL;
            break;
        case VAR_DECL:
            stmt->body._var_decl = malloc(sizeof(VarDeclStmt)); 
            stmt->body._var_decl->init_expr = NULL;
            stmt->body._var_decl->symbol = NULL;
            break;
    }
    return stmt;
}

void freeStmt(Stmt *stmt){
    int i;
    if(stmt == NULL) return;
    switch(stmt->type){
        case PRINT_STMT: 
            freeExpr(stmt->body._print->expr); 
            free(stmt->body._print);
            break;
        case EXPR_STMT: 
            freeExpr(stmt->body._expr->expr); 
            free(stmt->body._expr);
            break;
        case IF_STMT: 
            freeExpr(stmt->body._if->condition);
            freeStmt(stmt->body._if->then_branch);
            freeStmt(stmt->body._if->else_branch);
            free(stmt->body._if);
            break;
        case WHILE_STMT: 
            freeExpr(stmt->body._while->condition);
            freeStmt(stmt->body._while->body);
            freeStmt(stmt->body._while->increment);
            free(stmt->body._while);
            break;
        case BLOCK_STMT:
            freeStmtList(&stmt->body._block->stmt_list);
            free(stmt->body._block);
            break;
        case VAR_DECL:
            freeExpr(stmt->body._var_decl->init_expr);
            free(stmt->body._var_decl);
            break;
        case BREAK_STMT:
        case SKIP_STMT:
            break;
    }
    free(stmt);
}

StmtList newStmtList(){
    StmtList list;
    list.count = 0;
    list.stmt = NULL;
    return list;
}

void appendStmtList(StmtList* list, Stmt* stmt){
    if(!stmt) return;
    list->stmt = realloc(list->stmt, sizeof(Stmt*) * (list->count + 1));
    list->stmt[list->count] = stmt;
    list->count++;
}

void freeStmtList(StmtList* list){
    int i;
    if(list->stmt != NULL) {
        for(i = 0; i < list->count; i++)
            freeStmt(list->stmt[i]);
        free(list->stmt);
        list->stmt = NULL;
    }
    list->count = 0;
}