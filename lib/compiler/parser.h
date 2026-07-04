#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "compiler/token.h"
#include "compiler/statement.h"

typedef struct {
    Token *current;
    bool has_error;
    char err_msg[256];
} Parser;

extern Parser parser;

void initParser(Token *start);

static Token advance();
static Token previous();
static Token peek();
static bool isAtEnd();
static bool check(TokenType);
//static bool matchAny(int, ...);
static bool matchAny(TokenType type, ...);
static Token consume(TokenType, const char*);

StmtList parse();

static Stmt* parseDeclaration();
static Stmt* parseVarDecl();
static Stmt* parseStmt();
static Stmt* parsePrint();
static Stmt* parseBlock();
static Stmt* parseIf();
static Stmt* parseWhile();
static Stmt* parseFor();
static Stmt* parseSkip();
static Stmt* parseBreak();
static Stmt* parseExprStmt();

static Expr* parseExpr();
static Expr* parseAssignment();
static Expr* parseOr();
static Expr* parseAnd();
static Expr* parseEquality();
static Expr* parseComparison();
static Expr* parseTerm();
static Expr* parseFactor();
static Expr* parseUnary();
static Expr* parsePrimary();
static Expr* parseArray();
static Expr* parseGroup();
static Expr* parseLiteral();
static Expr* parseVarExpr();

#endif