#include "compiler/parser.h"
#include <stdarg.h>
#include <stdio.h>

#define PARSER_ERROR(...) do {snprintf(parser.err_msg, sizeof(parser.err_msg), __VA_ARGS__); parser.has_error = true;} while(false)
#define END_PARSING_IF_ERROR() if(parser.has_error) goto end_parsing
#define RETURN_STMT(stmt) \
                        do { \
                            if(parser.has_error) { \
                                freeStatement(stmt); \
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
    TokenType type;
    bool matched = false;

    va_start(vargs, type);
    do{
        if(check(type)) {
            advance();
            matched = true;
            break;
        }
        type = va_arg(vargs, TokenType);
    } while(type);

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
    }
    return parseStmt();
}

Stmt* parseVarDecl(){
    Token id;
    Expr* init_expr;
    Stmt* stmt = newStatement(VAR_DECL);

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

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseStmt(){
    if(match(TOKEN_IF)) return parseIf();
    if(match(TOKEN_PRINT)) return parsePrint();
    if(match(TOKEN_WHILE)) return parseWhile();
    if(match(TOKEN_FOR)) return parseFor();
    if(match(TOKEN_LEFT_BRACE)) return parseBlock();
    if(match(TOKEN_SKIP)) return parseSkip();
    if(match(TOKEN_BREAK)) return parseBreak();
    return parseExprStmt();    
}

Stmt* parsePrint(){
    Stmt* stmt = newStatement(PRINT_STMT);

    stmt->body._print->expr = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_SEMICOLON, "expect ';' after print statement");

    end_parsing:
        RETURN_STMT(stmt);

}
Stmt* parseBlock(){ 
    Stmt* stmt = newStatement(BLOCK_STMT);
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
    Stmt *stmt = newStatement(IF_STMT);
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
    Stmt *stmt = newStatement(WHILE_STMT);

    consume(TOKEN_LEFT_PAREN, "expect '('");
    END_PARSING_IF_ERROR();

    stmt->body._while->condition = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_RIGHT_PAREN, "expect ')'");
    END_PARSING_IF_ERROR();

    stmt->body._while->body = parseStmt();
    END_PARSING_IF_ERROR();

    end_parsing:
        RETURN_STMT(stmt);
}

Stmt* parseFor(){ // to be implemented
    Stmt* stmt = newStatement(BLOCK_STMT);
    Stmt* init = NULL;
    Expr* cond = NULL;
    Expr* increment = NULL;
    Stmt* body = NULL;
    Stmt* loop = NULL;

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

    appendStmtList(&stmt->body._block->stmt_list, init);
    loop = newStatement(WHILE_STMT);
    loop->body._while->condition = cond;
    loop->body._while->body = body;
    loop->body._while->increment = newStatement(EXPR_STMT);
    loop->body._while->increment->body._expr->expr = increment;
    appendStmtList(&stmt->body._block->stmt_list, loop);
    
    end_parsing:
        if(parser.has_error){
            freeStmt(init);
            freeExpr(cond);
            freeExpr(increment);
            freeStmt(body);
            freeStmt(stmt);
        }
        return stmt;
}

Stmt* parseSkip(){
    Stmt *stmt = newStatement(SKIP_STMT);
    consume(TOKEN_SEMICOLON, "expect ';' after skip");
    RETURN_STMT(stmt);
}

Stmt* parseBreak(){
    Stmt *stmt = newStatement(BREAK_STMT);
    consume(TOKEN_SEMICOLON, "expect ';' after break");
    RETURN_STMT(stmt);
}

Stmt* parseExprStmt(){
    Stmt *stmt = newStatement(EXPR_STMT);
    stmt->body._expr->expr = parseExpr();
    if(parser.has_error) {
        freeStatement(stmt);
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
    Expr* left;
    Expr* right;
    Token equal;

    expr = parseOr();
    END_PARSING_IF_ERROR();

    if(match(TOKEN_EQUAL)){
        left = expr;
        equal = previous();
        right = parseAssignment();
        END_PARSING_IF_ERROR();

        if(expr->type == VAR_EXPR) {
            expr = newExpr(ASSIGNMENT_EXPR);
            expr->body._assign->var = left;
            expr->body._assign->val = right;
        }
    }

    end_parsing: RETURN_EXPR(expr);
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
        right = parseOr();
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
        right = parseAnd();
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

    expr = parseTerm();
    END_PARSING_IF_ERROR();

    while(match(TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL)){
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
        right = parseTerm();
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

    while(match(TOKEN_STAR, TOKEN_SLASH)){
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
        return parsePrimary();
    }

    end_parsing: RETURN_EXPR(expr);
}

Expr* parsePrimary(){
    if(match(TOKEN_LEFT_PAREN)){
        return parseGroup();
    }
    else if(match(TOKEN_IDENTIFIER)){
        return parseVarExpr();
    }
    else {
        return parseLiteral();
    }
}

Expr* parseGroup(){
    Expr* expr;
    Expr* body;
    body = expr = parseExpr();
    END_PARSING_IF_ERROR();

    consume(TOKEN_RIGHT_PAREN, "Expect closing '('");
    END_PARSING_IF_ERROR();

    expr = newExpr(GROUP_EXPR);
    expr->body._group->expr = body;

    end_parsing: RETURN_EXPR(expr);
}

Expr* parseLiteral(){
    Token lit = previous();
    Expr *expr = newExpr(VAR_EXPR);
    expr->body._lit->token = lit;
    return expr;
}

Expr* parseVarExpr(){
    Token id = previous();
    Expr *expr = newExpr(VAR_EXPR);
    expr->body._var->name = id;
    return expr;
}