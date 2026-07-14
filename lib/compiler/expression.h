#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "compiler/token.h"
#include "compiler/resolve_scope.h"
#include "common.h"
#include "value.h"

struct _expression;
typedef struct _expression Expr;

typedef enum {
    LITERAL_EXPR,
    BINARY_EXPR,
    GROUP_EXPR,
    UNARY_EXPR,
    VAR_EXPR,
    ASSIGNMENT_EXPR,
    ARR_EXPR,
    CALL_EXPR,
    INDEX_EXPR
} ExprType;

typedef struct {
    Expr* left;
    Expr* right;
    Token op;
} BinaryExpr;

typedef struct {
    Token op;
    Expr* right;
} UnaryExpr;

typedef struct {
    Token token; 
} LiteralExpr;

typedef struct {
    Token name;
    Symbol* symbol;
} VarExpr;

typedef struct {
    Expr* expr;
} GroupExpr;

typedef struct {
    Expr* lval;
    Expr* rval;
} AssignmentExpr;

typedef struct {
    int count;
    Expr** elements;
} ArrExpr;

typedef struct {
    Expr* var;
    Expr* index;
} IndexExpr;

typedef struct {
    Expr* callee;
    size_t argc;
    struct {
        Expr** list;
        size_t capacity;
    } argv;
} CallExpr;

struct _expression {
    ExprType type;
    union {
        BinaryExpr* _bin;
        UnaryExpr* _unary;
        LiteralExpr* _lit;
        VarExpr* _var;  
        GroupExpr* _group;    
        AssignmentExpr* _assign;
        ArrExpr* _arr;
        CallExpr* _call;
        IndexExpr* _index;
    } body;
};

Expr* newExpr(ExprType);
void freeExpr(Expr*);
bool tokenToValue(ObjHeap* heap, Token token, Value* val_ptr);
ArrExpr* appendArrExpr(ArrExpr*, Expr*);
bool isConstantExpr(const Expr*);
void callExprInit(CallExpr*);
void callExprAddParam(CallExpr*, Expr*);

#endif