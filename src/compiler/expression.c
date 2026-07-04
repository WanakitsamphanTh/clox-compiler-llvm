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
        lexeme[token.length - 1] = '\0'; // remove the last "
        lexeme = decodeString(lexeme);
        char* literal = lexeme + 1;
        size_t len = strlen(literal); // remove the first "

        ObjString* string = makeObjString(literal, len);

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
            expr->body._assign->var.type = TOKEN_NONE;
            expr->body._assign->val = NULL;
            break;
        case ARR_EXPR:
            expr->body._arr = malloc(sizeof(ARR_EXPR));
            expr->body._arr->elements = NULL;
            expr->body._arr->count = 0;
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
            freeExpr(expr->body._assign->val);
            free(expr->body._assign);
            break;   
        case ARR_EXPR:{
            int i = 0;
            for(i = 0; i < expr->body._arr->count; i++) 
                freeExpr(expr->body._arr->elements[i]);
            free(expr->body._arr->elements);
            free(expr->body._arr);
            break; 
        }
    }

    free(expr);
}

ArrExpr* appendArrExpr(ArrExpr* expr, Expr* elem){
    Expr** new = realloc(expr->elements, sizeof(Expr*) * (expr->count+1));
    if(new == NULL) return NULL;
    new[expr->count++] = elem;
    expr->elements = new;
    return expr;
}

bool isConstantExpr(const Expr* expr){
    switch(expr->type){
        case BINARY_EXPR:
            return isConstantExpr(expr->body._bin->left) && isConstantExpr(expr->body._bin->right);
        case UNARY_EXPR:
            return isConstantExpr(expr->body._unary->right);
        case GROUP_EXPR:
            return isConstantExpr(expr->body._group->expr);
        case ARR_EXPR:{
            int i;
            for(i = 0; i < expr->body._arr->count; i++)
                if(!isConstantExpr(expr->body._arr->elements[i])) 
                    return false;
            return true;
        }
        case ASSIGNMENT_EXPR:{ /* determined by rvalue*/
            return isConstantExpr(expr->body._assign->val);
        }

        /* literal are constant */
        case LITERAL_EXPR:
            return true;

        /* variable involved -> not constant*/
        case VAR_EXPR:
            return false;
            break;
    }

    return false; /* unknown variable can not be fast-evaluated hence not constant*/
}