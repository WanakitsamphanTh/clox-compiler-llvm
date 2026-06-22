#include "compiler/expression.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool tokenToValue(Token token, Value *val_ptr){
    char* lexeme = getLexeme(token);
    char* tmp;
    TokenType type = token.type;
    bool successful = true;

    if(type == TOKEN_STRING) {
        val_ptr->type = STR_VALUE;
        tmp = malloc(token.length + 1 - 2);
        lexeme[token.length - 1] = '\0'; // remove final '"'
        strcpy(tmp, lexeme + 1); // ignore first '"'
        val_ptr->val.str = tmp;

    } else if(type == TOKEN_NUMBER) {
        val_ptr->type = NUMBER_VALUE;
        val_ptr->val.num = strtod(lexeme, NULL);

    } else successful = false;
    
    free(lexeme);
    return successful;
}

Expr* newExpr(ExprType type){
    Expr* expr = malloc(sizeof(Expr));
    switch(type){
        case LITERAL_EXPR: break;
        case BINARY_EXPR: 
            expr->body._bin = malloc(sizeof(BinaryExpr));
            expr->body._bin->left = expr->body._bin->right = NULL;
            expr->body._bin->op.type = TOKEN_EOF;
            break;
        case GROUP_EXPR: 
            expr->body._group = malloc(sizeof(GroupExpr));
            expr->body._group->expr = NULL;
            break;
        case UNARY_EXPR: 
            expr->body._unary = malloc(sizeof(UnaryExpr));
            expr->body._unary->right = NULL;
            expr->body._unary->op.type = TOKEN_EOF;
            break;
        case VAR_EXPR: 
            expr->body._var = malloc(sizeof(VarExpr));
            expr->body._var->name.type = TOKEN_EOF;
            break;
        case ASSIGNMENT_EXPR:
            expr->body._var = malloc(sizeof(AssignmentExpr));
            expr->body._assign->var = expr->body._assign->val = NULL;
            break;
    }
    return expr;
}

void freeExpr(Expr *expr){
    if(expr == NULL) return;

    switch(expr->type){
        case LITERAL_EXPR: 
            break;
        case BINARY_EXPR: 
            freeExpr(expr->body._bin->left);
            freeExpr(expr->body._bin->right);
            free(expr->body._bin);
            break;
        case UNARY_EXPR:
            freeExpr(expr->body._unary->right);
            free(expr->body._unary);
            break;
        case GROUP_EXPR: 
            freeExpr(expr->body._group->expr);
            free(expr->body._group);
            break;
        case VAR_EXPR: 
            free(expr->body._var);
            break;    
        case ASSIGNMENT_EXPR:
            freeExpr(expr->body._assign->var);
            freeExpr(expr->body._assign->val);
            free(expr->body._assign);
            break;    
    }

    free(expr);
}