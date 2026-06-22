#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "compiler/token.h"
#include "common.h"

struct _expression;
typedef struct _expression Expr;

typedef enum {
    NUMBER_VALUE,
    STR_VALUE,
    BOOL_VALUE
} ValueType;

typedef struct {
    ValueType type;
    union {
        double num;
        bool b;
        const char* str;
    } val;
} Value;

typedef enum {
    LITERAL_EXPR,
    BINARY_EXPR,
    GROUP_EXPR,
    UNARY_EXPR,
    VAR_EXPR,
    ASSIGNMENT_EXPR,
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
} VarExpr;

typedef struct {
    Expr* expr;
} GroupExpr;

typedef struct {
    Expr* var;
    Expr* val;
} AssignmentExpr;

struct _expression {
    ExprType type;
    union {
        BinaryExpr* _bin;
        UnaryExpr* _unary;
        LiteralExpr* _lit;
        VarExpr* _var;  
        GroupExpr* _group;    
        AssignmentExpr* _assign;
    } body;
};

Expr* newExpr(ExprType);
void freeExpr(Expr*);
bool tokenToValue(Token token, Value* val_ptr);

#endif