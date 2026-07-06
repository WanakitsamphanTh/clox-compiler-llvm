#ifndef COMPILER_H
#define COMPILER_H
#include "vm.h"
#include "common.h"
#include "compiler/statement.h"
#include "compiler/expression.h"
#include "compiler/resolve_loop.h"
#include "compiler/resolve_scope.h"

extern Chunk* compilingChunk;
extern bool compile_error;
extern char compile_error_msg[256];

static void resolve(StmtList* stmt);

static void compileOperator(TokenType, ExprType);
static void compileStatementList(StmtList*);
static void compileStatement(Stmt*);
static void compileExpr(Expr*);

static void emitBytes(int,...);
static Chunk* currentChunk();

bool compile(const char*, Chunk*);
static bool emitConstant(Value);
static uint8_t makeConstant(Value);
static uint8_t makeIdentifierConstant(const char*, int);
static void defineVariable(uint8_t);

static int emitJump(uint8_t);
static void patchJump(int);
static void emitLoop(int);

#define emitByte(b) emitBytes(1, b)


typedef struct {
    ScopeResolver resolver;
    LoopStack loops;

    Chunk* compilingChunk;

    int depth;
} Compiler;

extern Compiler compiler;

static void initCompiler();
static void freeCompiler();

static bool _debugStmtList(StmtList stmts, int indent);
static bool _debugStmt(Stmt* stmt, int indent);
static bool _debugExpr(Expr* expr, int indent);

#endif