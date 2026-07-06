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
    ARR_EXPR
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
    Token var;
    Expr* val;
} AssignmentExpr;

typedef struct {
    int count;
    Expr** elements;
} ArrExpr;

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
    } body;
};

Expr* newExpr(ExprType);
void freeExpr(Expr*);
bool tokenToValue(Token token, Value* val_ptr);
ArrExpr* appendArrExpr(ArrExpr*, Expr*);
bool isConstantExpr(const Expr*);

#endif