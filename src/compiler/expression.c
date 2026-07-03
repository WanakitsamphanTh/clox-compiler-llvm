#include "compiler/expression.h"
#include "value.h"
#include "vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool tokenToValue(Token token, Value *val_ptr){
    char* lexeme = getLexeme(token);
    TokenType type = token.type;
    bool successful = true;

    if(type == TOKEN_STRING) {
       /*int len = token.length - 2;
       char* temp = malloc(len + 1);
       lexeme[token.length - 1] = '\0';
       strcpy(temp, lexeme + 1);
       //memcpy(temp, lexeme+1, len);
       temp = decodeString(temp);
       len = strlen(temp);


       uint32_t hash = hashString(temp, len);
       ObjString* string = newObjString(temp, len, hash);
       free(temp);
       val_ptr->type = OBJ_VALUE;
       val_ptr->val.obj = string;*/

        lexeme[token.length - 1] = '\0'; // remove the last "
        lexeme = decodeString(lexeme);
        char* literal = lexeme + 1;
        size_t len = strlen(literal); // remove the first "

        uint32_t hash = hashString(literal, len);
        ObjString* string = tableFindString(&vm.strings, literal, len, hash);
        if(string == NULL) string = newObjString(literal, len, hash);

        val_ptr->type = OBJ_VALUE;
        val_ptr->val.obj = string;
       
    } else if(type == TOKEN_NUMBER) {
        val_ptr->type = NUMBER_VALUE;
        val_ptr->val.num = strtod(lexeme, NULL);
    } else if(type == TOKEN_TRUE) {
        val_ptr->type = BOOL_VALUE;
        val_ptr->val.b = true;
    }
    else if(type == TOKEN_FALSE) {
        val_ptr->type = BOOL_VALUE;
        val_ptr->val.b = false;
    }
    else if(type == TOKEN_NIL){
        val_ptr->type = NIL_VALUE;
    }
    else successful = false;
    
    free(lexeme);
    return successful;
}

Expr* newExpr(ExprType type){
    Expr* expr = malloc(sizeof(Expr));
    expr->type = type;
    switch(type){
        case LITERAL_EXPR:
            expr->body._lit = malloc(sizeof(LiteralExpr));
            expr->body._lit->token.type = TOKEN_NONE;
            break;
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
            expr->body._assign = malloc(sizeof(AssignmentExpr));
            expr->body._assign->var = NULL;
            expr->body._assign->val = NULL;
            break;
        default:
            free(expr);
            return NULL;
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