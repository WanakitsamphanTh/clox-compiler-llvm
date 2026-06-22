#ifndef STATEMENT_H
#define STATEMENT_H

#include "common.h"
#include "compiler/expression.h"

struct _Stmt;
typedef struct _Stmt Stmt;

typedef struct {
    int count;
    Stmt** stmt;
} StmtList;

typedef enum {
    PRINT_STMT,
    EXPR_STMT, 

    // Flow control statements
    IF_STMT,
    WHILE_STMT,
    BREAK_STMT,
    SKIP_STMT,
    BLOCK_STMT,

    // Declaration
    VAR_DECL,  
} StmtType;

typedef struct{
    Expr *expr;
} PrintStmt;

typedef struct {
    Expr *expr;
} ExprStmt;

typedef struct{
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} IfStmt;

typedef struct{
    Expr *condition;
    Stmt *body;
    Stmt *increment;
} WhileStmt;

typedef struct{
    StmtList stmt_list;
} BlockStmt;

typedef struct {
    Token name;
    Expr *init_expr;
} VarDeclStmt;

struct _Stmt {
    StmtType type;
    union {
        PrintStmt* _print;
        ExprStmt* _expr;
        BlockStmt* _block;
        IfStmt* _if;
        WhileStmt* _while;
        VarDeclStmt* _var_decl;
    } body;
};


Stmt* newStatement(StmtType);
void freeStatement(Stmt*);
void freeStatementList(Stmt**, int);

StmtList newStmtList();
void appendStmtList(StmtList*, Stmt*);
void freeStmtList(StmtList*);

#endif