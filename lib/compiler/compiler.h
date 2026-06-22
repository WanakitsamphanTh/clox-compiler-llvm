#ifndef COMPILER_H
#define COMPILER_H
#include "vm.h"
#include "common.h"
#include "compiler/statement.h"
#include "compiler/expression.h"


bool compileStatements(Stmt*, Chunk*);
bool compileExpr(Expr*, Chunk*);
bool compile(const char*, Chunk*);

#endif