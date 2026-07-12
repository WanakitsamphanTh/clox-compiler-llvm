#include "compiler/parser.h"
#include "compiler/statement.h"
#include "compiler/expression.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define match(...) matchAny(__VA_ARGS__,TOKEN_NONE)

#define PARSER_ERROR(...) do {snprintf(parser.err_msg, sizeof(parser.err_msg), __VA_ARGS__); parser.has_error = true;} while(false)
#define END_PARSING_IF_ERROR() if(parser.has_error) goto end_parsing
#define RETURN_STMT(stmt) \
                        do { \
                            if(parser.has_error) { \
                                freeStmt(stmt); \
                                return NULL; \
                            } \
                            return stmt; \
                        } while(0)
#define RETURN_EXPR(expr) \
                        do { \
                            if(parser.has_error) { \
                                freeExpr(expr); \
                                return NULL; \
                            } \
                            return expr; \
                        } while(0)

Parser parser;

void initParser(Token *start){
    parser.current = start;
    parser.err_msg[0] = '\0';
    parser.has_error = false;
}

Token advance(){
    if(!isAtEnd()){
        return *(parser.current++);
    } 
    return *(parser.current);
}
Token previous(){
    return *(parser.current - 1);
}
Token peek(){
    return *(parser.current);
}
bool isAtEnd(){
    return parser.current->type == TOKEN_EOF;
}

bool check(TokenType t){
    return parser.current->type == t;
}

/*bool matchAny(int n, ...){
    int i;
    va_list vargs;
    TokenType type;
    bool matched = false;

    va_start(vargs, n);
    for(i = 0; i < n; i++){
        type = va_arg(vargs, TokenType);
        if(check(type)) {
            advance();
            matched = true;
            break;
        }
    }
    va_end(vargs);
    return matched;
}*/

static bool matchAny(TokenType type, ...){
    int i;
    va_list vargs;
    bool matched = false;

    
    va_start(vargs, type);
    do{
        if(check(type)) {
            advance();
            matched = true;
            break;
        }
        type = (TokenType) va_arg(vargs, TokenType);
    } while(type != TOKEN_NONE);

    va_end(vargs);
    return matched;
}


Token consume(TokenType type, const char* msg){
    if(check(type)) {
        return advance();
    }
    PARSER_ERROR("Error at line %2d: %s", peek().line, msg);
    return peek();
}

StmtList parse(){
    Stmt* stmt;
    StmtList statements = newStmtList();

    while(!isAtEnd()) {
        stmt = parseDeclaration();
        if(parser.has_error) break;
        appendStmtList(&statements, stmt);
    }

    return statements;
}

Stmt* parseDeclaration(){
    if(match(TOKEN_VAR)) {
        return parseVarDecl();
    } else if(match(TOKEN_FUN)){
        return parseFnDecl();
    }
    return parseStmt();
}

Stmt* parseVarDecl(){
    Token id;
    Expr* init_expr;
    Stmt* stmt = newStmt(VAR_DECL);

    id = consume(TOKEN_IDENTIFIER, "expect variable name");
    END_PARSING_IF_ERROR();
    stmt->body._var_decl->name = id;

    // match '='
    if(match(TOKEN_EQUAL)){
        init_expr = parseExpr();
        END_PARSING_IF_ERROR();
        stmt->body._var_decl->init_expr = init_expr;
    }

    consume(TOKEN_SEMICOLON, "expect semicolon after variable declaration");
    END_PARSING_IF_ERROR();

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseFnDecl(){
    Token id;
    Stmt* stmt = newStmt(FN_DECL);

    id = consume(TOKEN_IDENTIFIER, "expect function name");
    END_PARSING_IF_ERROR();
    stmt->body._fn_decl->name = id;

    consume(TOKEN_LEFT_PAREN, "expect open '(' after function name"); END_PARSING_IF_ERROR();
    if(!check(TOKEN_RIGHT_PAREN)){
        do {
            id = consume(TOKEN_IDENTIFIER, "expect variable name");
            fnDeclAddParam(stmt->body._fn_decl, id);
        } while(match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "expect closing ')' after parameters"); END_PARSING_IF_ERROR();
    consume(TOKEN_LEFT_BRACE, "Expect '{' after function signature"); END_PARSING_IF_ERROR();
    stmt->body._fn_decl->body = parseBlock(); END_PARSING_IF_ERROR();

    end_parsing:
    if(parser.has_error) {
        freeStmt(stmt);
        return NULL;
        } 
    return stmt;
}

Stmt* parseStmt(){
    if(match(TOKEN_IF)) return parseIf();
    if(match(TOKEN_PRINT)) return parsePrint();
    if(match(TOKEN_WHILE)) return parseWhile();
    if(match(TOKEN_FOR)) return parseFor();
    if(match(TOKEN_LEFT_BRACE)) return parseBlock();
    if(match(TOKEN_SKIP)) return parseSkip();
    if(match(TOKEN_BREAK)) return parseBreak();
    if(match(TOKEN_RETURN)) return parseReturn();
    return parseExprStmt();    
}

Stmt* parseReturn(){
    Stmt* stmt = newStmt(RETURN_STMT);
    if(!check(TOKEN_SEMICOLON)){
        stmt->body._ret->ret_val = parseExpr();
        END_PARSING_IF_ERROR();
    }
    consume(TOKEN_SEMICOLON, "Expect semicolon after return");
    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parsePrint(){
    Stmt* stmt = newStmt(PRINT_STMT);

    stmt->body._print->expr = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_SEMICOLON, "expect ';' after print statement");

    end_parsing:
        RETURN_STMT(stmt);

}
Stmt* parseBlock(){ 
    Stmt* stmt = newStmt(BLOCK_STMT);
    Stmt* sub_stmt;
    while(!check(TOKEN_RIGHT_BRACE) && !isAtEnd()){
        sub_stmt = parseDeclaration();
        END_PARSING_IF_ERROR();
        appendStmtList(&stmt->body._block->stmt_list, sub_stmt);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' to close scope.");
    END_PARSING_IF_ERROR();

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseIf(){
    Stmt *stmt = newStmt(IF_STMT);
    consume(TOKEN_LEFT_PAREN, "expect '('");
    END_PARSING_IF_ERROR();

    stmt->body._if->condition = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_RIGHT_PAREN, "expect ')'");
    END_PARSING_IF_ERROR();

    stmt->body._if->then_branch = parseStmt();
    END_PARSING_IF_ERROR();

    if(match(TOKEN_ELSE)) {
        stmt->body._if->else_branch = parseStmt();
        END_PARSING_IF_ERROR();
    }

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseWhile(){
    Stmt *stmt = newStmt(WHILE_STMT);

    consume(TOKEN_LEFT_PAREN, "expect '('");
    END_PARSING_IF_ERROR();

    stmt->body._while->condition = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_RIGHT_PAREN, "expect ')'");
    END_PARSING_IF_ERROR();

    stmt->body._while->body = parseStmt();
    END_PARSING_IF_ERROR();
    
    stmt->body._while->increment = NULL;

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseFor(){ // to be implemented
    Stmt* stmt = newStmt(BLOCK_STMT);
    Stmt* init = NULL;
    Expr* cond = NULL;
    Expr* increment = NULL;
    Stmt* body = NULL;
    Stmt* loop = NULL;

    consume(TOKEN_LEFT_PAREN, "expect '(' after for");
    END_PARSING_IF_ERROR();

    if(match(TOKEN_SEMICOLON));
    else if(match(TOKEN_VAR)){
        init = parseVarDecl();
        END_PARSING_IF_ERROR();
    } else {
        init = parseExprStmt();
    }

    if(!check(TOKEN_SEMICOLON)){
        cond = parseExpr();
    }
    consume(TOKEN_SEMICOLON, "expect ';' after condition in for statement");
    END_PARSING_IF_ERROR();

    if(!check(TOKEN_RIGHT_PAREN)){
        increment = parseExpr();
        END_PARSING_IF_ERROR();
    }
    consume(TOKEN_RIGHT_PAREN, "expect closing ')'");
    END_PARSING_IF_ERROR();

    body = parseStmt();
    END_PARSING_IF_ERROR();

    appendStmtList(&stmt->body._block->stmt_list, init); init = NULL;
    loop = newStmt(WHILE_STMT);
    loop->body._while->condition = cond; cond = NULL;
    loop->body._while->body = body; body = NULL;
    loop->body._while->increment = newStmt(EXPR_STMT);
    loop->body._while->increment->body._expr->expr = increment; increment = NULL;
    appendStmtList(&stmt->body._block->stmt_list, loop); loop = NULL;
    
    end_parsing:
        if(parser.has_error){
            freeStmt(init);
            freeExpr(cond);
            freeExpr(increment);
            freeStmt(body);
            freeStmt(stmt);
            return NULL;
        }
        return stmt;
}

Stmt* parseSkip(){
    Stmt *stmt = newStmt(SKIP_STMT);
    consume(TOKEN_SEMICOLON, "expect ';' after skip");
    RETURN_STMT(stmt);
}

Stmt* parseBreak(){
    Stmt *stmt = newStmt(BREAK_STMT);
    consume(TOKEN_SEMICOLON, "expect ';' after break");
    RETURN_STMT(stmt);
}

Stmt* parseExprStmt(){
    Stmt *stmt = newStmt(EXPR_STMT);
    stmt->body._expr->expr = parseExpr();
    if(parser.has_error) {
        freeStmt(stmt);
        return NULL;
    }
    consume(TOKEN_SEMICOLON, "expect ';' after expression");
    RETURN_STMT(stmt);
}

Expr* parseExpr() { //to be implemented
    return parseAssignment();
}

Expr* parseAssignment(){
    Expr* expr;
    Expr* left = NULL;
    Expr* right = NULL;
    Token equal;

    expr = parseOr();
    END_PARSING_IF_ERROR();

    if(match(TOKEN_EQUAL)){
        left = expr;
        if(left->type == VAR_EXPR){
            right = parseAssignment();
            END_PARSING_IF_ERROR();
            expr = newExpr(ASSIGNMENT_EXPR);
            expr->body._assign->val = right;
            expr->body._assign->var = left->body._var->name;
            right = NULL;
            freeExpr(left);
        } else {
            PARSER_ERROR("Invalid assignment target");
        }

    }

    end_parsing: 
        if(parser.has_error){
            if(left != expr) freeExpr(left);
            freeExpr(right);
            freeExpr(expr);
            return NULL; 
        }
        return expr;
}

Expr* parseOr(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseAnd();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_OR)){
        left = expr;
        op = previous();
        right = parseAnd();
        END_PARSING_IF_ERROR();
        
        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseAnd(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseEquality();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_AND)){
        left = expr;
        op = previous();
        right = parseEquality();
        END_PARSING_IF_ERROR();
        
        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseEquality(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseComparison();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL)){
        left = expr;
        op = previous();
        right = parseComparison();
        END_PARSING_IF_ERROR();

        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);
}

static Expr* parseComparison(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseTerm();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL)){
        left = expr;
        op = previous();
        right = parseTerm();
        END_PARSING_IF_ERROR();

        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);   
}

Expr* parseTerm(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseFactor();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_PLUS, TOKEN_MINUS)){
        left = expr;
        op = previous();
        right = parseFactor();
        END_PARSING_IF_ERROR();

        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseFactor(){
    Expr *expr;
    Expr *left;
    Expr *right;
    Token op;

    expr = parseUnary();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT)){
        left = expr;
        op = previous();
        right = parseUnary();
        END_PARSING_IF_ERROR();

        expr = newExpr(BINARY_EXPR);
        expr->body._bin->op = op;
        expr->body._bin->left = left;
        expr->body._bin->right = right;
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseUnary() {
    Expr *expr;
    Expr *right;
    Token op;

    if(match(TOKEN_BANG, TOKEN_MINUS)){
        op = previous();
        right = expr = parseUnary();
        END_PARSING_IF_ERROR();
        expr = newExpr(UNARY_EXPR);
        expr->body._unary->op = op;
        expr->body._unary->right = right;
    } else {
        return parseCallExpr();
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseCallExpr(){
    Expr* expr = parsePrimary(); END_PARSING_IF_ERROR();
    Expr* arg = NULL;
    Expr* callee = NULL;
    while(match(TOKEN_LEFT_PAREN)){
        callee = expr;
        expr = newExpr(CALL_EXPR);
        expr->body._call->callee = callee;
        callee = NULL;
        if(!check(TOKEN_RIGHT_PAREN)){
            int i = 0;
            do{
                arg = parseExpr(); END_PARSING_IF_ERROR();
                callExprAddParam(expr->body._call, arg);
                arg = NULL;
            } while(match(TOKEN_COMMA));
        }
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
        END_PARSING_IF_ERROR();
    }
    end_parsing:
    if(parser.has_error) {
        freeExpr(expr);
        freeExpr(callee);
        freeExpr(arg);
        return NULL;
    }
    return expr;
}

Expr* parsePrimary(){
    if(match(TOKEN_LEFT_PAREN)){
        return parseGroup();
    }
    else if(match(TOKEN_IDENTIFIER)){
        return parseVarExpr();
    } else if(match(TOKEN_NUMBER, TOKEN_STRING, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL)){
        return parseLiteral();
    } else if(match(TOKEN_LEFT_BRACE)){
        return parseArray();
    } else {
        char buffer[32];
        memcpy(buffer, parser.current->start, parser.current->length);
        PARSER_ERROR("Invalid token '%s' at line %d", buffer,  parser.current->line);
        return NULL;
    }
}

Expr* parseArray(){
    Expr* elem = NULL;
    Expr* expr = newExpr(ARR_EXPR); 
    END_PARSING_IF_ERROR();

    if(!check(TOKEN_RIGHT_BRACE))
        for(;;){
            elem = parseExpr();
            END_PARSING_IF_ERROR();
            
            ArrExpr* tmp = appendArrExpr(expr->body._arr, elem);
            if(tmp == NULL) {
                PARSER_ERROR("cannot append array expression (returns NULL)");
                goto end_parsing;
            }
            
            elem = NULL;    // transfered ownership

            if(!match(TOKEN_COMMA)) break;
        }

    consume(TOKEN_RIGHT_BRACE, "Expect closing '}'");
    END_PARSING_IF_ERROR();

    end_parsing:
        if(parser.has_error){
            freeExpr(elem);
            freeExpr(expr);
        }
        return expr;
}

Expr* parseGroup(){
    Expr* expr;
    Expr* body;
    body = expr = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_RIGHT_PAREN, "Expect closing ')'");
    END_PARSING_IF_ERROR();

    expr = newExpr(GROUP_EXPR);
    expr->body._group->expr = body;

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseLiteral(){
    Token lit = previous();
    Expr *expr = newExpr(LITERAL_EXPR);
    expr->body._lit->token = lit;
    return expr;
}

Expr* parseVarExpr(){
    Token id = previous();
    Expr *expr = newExpr(VAR_EXPR);
    expr->body._var->name = id;
    return expr;
}
