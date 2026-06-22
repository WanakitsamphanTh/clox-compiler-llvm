#ifndef COMPILER_H
#define COMPILER_H
#include "vm.h"
#include "common.h"
#include "compiler/statement.h"
#include "compiler/expression.h"

extern Chunk* compilingChunk;
extern bool compile_error;
extern char compile_error_msg[256];

static bool compileOperator(TokenType, ExprType);
static bool compileStatementList(StmtList*);
static bool compileStatement(Stmt*);
static bool compileExpr(Expr*);

static void emitBytes(int,...);
static Chunk* currentChunk();

bool compile(const char*, Chunk*);
bool emitConstant(Value);
uint8_t makeConstant(Value);

#define emitByte(b) emitBytes(1, b)

static bool _debugStmtList(StmtList stmts, int indent);
static bool _debugStmt(Stmt* stmt, int indent);
static bool _debugExpr(Expr* expr, int indent);

#endif