#include "compiler/compiler.h"
#include "compiler/scanner.h"
#include "compiler/token.h"
#include "compiler/parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define _debugAst(stmt_list) _debugStmtList(stmt_list, 0);

Chunk* compilingChunk;
bool compile_error = false;
char compile_error_msg[256];

#define COMPILE_ERROR(...) do{ compile_error = true; sprintf(compile_error_msg, __VA_ARGS__); }while(false)

bool compile(const char* source, Chunk* chunk){
    int line = -1;
    char *lexeme;
    int i;
    Token token;
    StmtList statements = {0, NULL};
    TokenList token_list = newTokenList();
    bool successful = true;

    // Scan and tokenization
    initScanner(source);

    //set compiling chunk
    compilingChunk = chunk;

    //scanning token
    while(1){
        token = scanToken();
        if(token.type == TOKEN_ERROR) {
            successful = false;
            goto compilation_terminated;
        }
        appendTokenList(&token_list, token);
        if(token.type == TOKEN_EOF) break;
    }

    // Parsing and AST generation
    initParser(token_list.tokens);
    statements = parse();

    if(parser.has_error) {
        fprintf(stderr, "Parser error: %s\n", parser.err_msg);
        successful = false;
        goto compilation_terminated;
    }

    #ifdef DEBUG_PRINT_AST
    _debugAst(statements);
    #endif

    // Compilation
    compileStatementList(&statements);
    if(compile_error){
        fprintf(stderr, "Compilation error: %s\n", compile_error_msg);
        successful = false;
        goto compilation_terminated;
    }

    writeChunk(chunk, OP_RETURN);

    //Free memory on termination
    compilation_terminated:
        #ifdef DEBUG_PRINT_CHUNK
        disassembleChunk(currentChunk(), "Main chunk");
        #endif
        freeStmtList(&statements); 
        freeTokenList(&token_list);

    return successful;   
}

void compileStatementList(StmtList* list){
    if(compile_error) return;
    int i;

    for(i = 0; i < list->count; i++){
        compileStatement(list->stmt[i]);
        if(compile_error)
            return;
    }

    return;
}

//#define TRY successful =
#define TERMINATE_IF_ERROR() if(compile_error) goto terminate_compilation

void compileStatement(Stmt* stmt){
    if(compile_error) return;

    switch(stmt->type){
        case BLOCK_STMT:
            compileStatementList(&stmt->body._block->stmt_list); TERMINATE_IF_ERROR();
            break;

        case EXPR_STMT: 
            compileExpr(stmt->body._expr->expr); TERMINATE_IF_ERROR();
            emitByte(OP_POP);
            break;

        case PRINT_STMT:
            compileExpr(stmt->body._print->expr); TERMINATE_IF_ERROR();
            emitByte(OP_PRINT);
            break;

        case VAR_DECL:
            /* emit rvalue*/
            if(stmt->body._var_decl->init_expr){
                compileExpr(stmt->body._var_decl->init_expr); TERMINATE_IF_ERROR();
            }
            else emitByte(OP_NIL);

            /*define variable*/
            const char* name = stmt->body._var_decl->name.start;
            int length = stmt->body._var_decl->name.length;
            char* lexeme = getLexeme(stmt->body._var_decl->name);
            uint8_t global = makeIdentifierConstant(name, length);
            defineVariable(global);
            break;
        
        case IF_STMT:{
            compileExpr(stmt->body._if->condition); TERMINATE_IF_ERROR();
            int jmp = emitJump(OP_JIF);
            emitByte(OP_POP);
            compileStatement(stmt->body._if->then_branch); TERMINATE_IF_ERROR();
                        
            int else_jmp = emitJump(OP_JMP);
            patchJump(jmp);
            /*if(stmt->body._if->else_branch){
                emitByte(OP_POP);
                compileStatement(stmt->body._if->else_branch); TERMINATE_IF_ERROR();
            }*/
            patchJump(else_jmp);
            
            break;
        }

        case WHILE_STMT: {
            int loopStart = currentChunk()->count;
            compileExpr(stmt->body._while->condition); TERMINATE_IF_ERROR();

            int exitJump = emitJump(OP_JIF);
            emitByte(OP_POP);

            compileStatement(stmt->body._while->body); TERMINATE_IF_ERROR();

            if(stmt->body._while->increment != NULL) {
                compileStatement(stmt->body._while->increment);
                TERMINATE_IF_ERROR();
            }

            emitLoop(loopStart);

            patchJump(exitJump);
            emitByte(OP_POP);
            break;
        }
        
        default:
            COMPILE_ERROR("Unimplemented statement %d", stmt->type);
    }

    terminate_compilation:
        return;
}

void compileExpr(Expr* expr){
    if(compile_error) return;
    
    Chunk *chunk = currentChunk();
    switch(expr->type){
        case LITERAL_EXPR:{
            switch(expr->body._lit->token.type){
                case TOKEN_TRUE:
                    emitByte(OP_TRUE); break;
                case TOKEN_FALSE:
                    emitByte(OP_FALSE); break;
                case TOKEN_NIL:
                    emitByte(OP_NIL); break;
                default:{
                    Value value;
                    tokenToValue(expr->body._lit->token, &value); TERMINATE_IF_ERROR();
                    emitConstant(value);
                }
            }
            break;
        }
        case VAR_EXPR: {
            const char* name = expr->body._var->name.start;
            int len = expr->body._var->name.length;
            uint8_t ref = makeIdentifierConstant(name, len);
            emitBytes(2, OP_LOAD_GLOBAL, ref);
            break;
        }
        case GROUP_EXPR: // to be implemented
            compileExpr(expr->body._group->expr); TERMINATE_IF_ERROR();
            break;
        case BINARY_EXPR:
            compileExpr(expr->body._bin->left); TERMINATE_IF_ERROR();
            compileExpr(expr->body._bin->right); TERMINATE_IF_ERROR();
            compileOperator(expr->body._bin->op.type, expr->type); TERMINATE_IF_ERROR();
            break;
        case UNARY_EXPR: 
            compileExpr(expr->body._unary->right); TERMINATE_IF_ERROR();
            compileOperator(expr->body._unary->op.type, expr->type); TERMINATE_IF_ERROR();
            break;
        case ASSIGNMENT_EXPR: { /* to implement*/
            compileExpr(expr->body._assign->val); TERMINATE_IF_ERROR();
            const char* name = expr->body._assign->var.start;
            int len = expr->body._assign->var.length;
            uint8_t ref = makeIdentifierConstant(name, len);
            emitBytes(2, OP_STORE_GLOBAL, ref);
            break;
        }
    }

    terminate_compilation:
        return;
}

void compileOperator(TokenType op, ExprType expr_type){
    if(compile_error) return;

    switch(op){
        case TOKEN_PLUS:
            emitByte(OP_ADD); break;
        case TOKEN_MINUS:
            if(expr_type == BINARY_EXPR)
                emitByte(OP_SUB); 
            else if(expr_type == UNARY_EXPR)
                emitByte(OP_NEGATE);
            else {
                COMPILE_ERROR("Operator '-' in an invalid expression");
                return;
            }
            break;
        case TOKEN_STAR:
            emitByte(OP_MULT);  break;
        case TOKEN_SLASH:
            emitByte(OP_DIV); break;
        
        case TOKEN_AND:
            emitByte(OP_AND); break;
        case TOKEN_OR:
            emitByte(OP_OR); break;
        case TOKEN_BANG:
            emitByte(OP_CMPL); break;
        case TOKEN_LESS:
            emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:
            emitByte(OP_LESS_EQ); break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:
            emitByte(OP_GREATER_EQ); break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQ); break;
        case TOKEN_BANG_EQUAL:
            emitBytes(2, OP_EQ, OP_CMPL); break;
        
        default:
            COMPILE_ERROR("Unknown parameter %d", op);
            return;
    }
    return;
}


void emitBytes(int n, ...){
    va_list bytes;
    uint8_t byte;
    int i;

    va_start(bytes, n);
    for(i = 0; i < n; i++){
        byte = (uint8_t) va_arg(bytes, int); //uint8_t is promoted in int when parsing to ...
        writeChunk(currentChunk(), byte);
    }
    
    va_end(bytes);
}

bool emitConstant(Value value){
    emitBytes(2, OP_CONST, makeConstant(value));
}

uint8_t makeConstant(Value value){
    uint8_t constant = addConstant(currentChunk(), value);        
    if (constant > UINT8_MAX) {
        COMPILE_ERROR("Too many constants in one chunk.");
        return 0;
    }
    return constant;
}

uint8_t makeIdentifierConstant(const char* name, int len){
    ObjString *var_name = makeObjString(name, len);
    return makeConstant(VALUE_OBJ(var_name));
}

 void defineVariable(uint8_t global){
    emitBytes(2, OP_DEFINE_GLOBAL, global);
}

int emitJump(uint8_t op){
    emitByte(op);
    emitBytes(2,0xff,0xff);
    return currentChunk()->count - 2;
}

void emitLoop(int loop_start){
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        COMPILE_ERROR("Loop body too large.");
        return;
    }
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

void patchJump(int offset){
    int jump = currentChunk()->count - offset - 2;

    if(jump > UINT16_MAX) {
        COMPILE_ERROR("Too many instructions to jump over");
        return;
    }

    currentChunk()->code[offset] = (uint8_t) (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = (uint8_t) jump & 0xff;         /* big endian?*/
}

Chunk* currentChunk(){
    return compilingChunk;
}

bool _debugStmtList(StmtList stmts, int indent){
    int i;
    for(i = 0; i < stmts.count; i++){
        _debugStmt(stmts.stmt[i], indent);
    }
}

void _putIndent(int indent){
    //put indent
    while(indent > 0){
        putchar(' ');
        indent--;
    }
}

static bool _debugStmt(Stmt* stmt, int indent){
    if(stmt == NULL) {
         _putIndent(indent);  printf("Null statement:\n");
         return false;
    }
    switch(stmt->type){
        case PRINT_STMT: 
            _putIndent(indent);  printf("Print statement:\n");
            _debugExpr(stmt->body._print->expr, indent + 1);
            break;
        case EXPR_STMT: 
            printf("Expression statement:\n");
            _debugExpr(stmt->body._expr->expr, indent + 1);
            break;

        // Flow control statements
        case IF_STMT: 
            _putIndent(indent++);  printf("If statement:\n");
            _putIndent(indent);  printf("Condition:\n");
            _debugExpr(stmt->body._if->condition, indent + 1);
            _putIndent(indent);  printf("Then:\n");
            _debugStmt(stmt->body._if->then_branch, indent + 1);
            _putIndent(indent);  printf("Else:\n");
            _debugStmt(stmt->body._if->else_branch, indent + 1);
            break;
        case WHILE_STMT: 
            _putIndent(indent++);  printf("Loop statement:\n");
            _putIndent(indent);  printf("Condition:\n");
            _debugExpr(stmt->body._while->condition, indent + 1);
            _putIndent(indent);  printf("Body:\n");
            _debugStmt(stmt->body._while->body, indent + 1);
            _putIndent(indent);  printf("Increment:\n");
            _debugStmt(stmt->body._while->increment, indent + 1);
            break;
        case BREAK_STMT: 
            _putIndent(indent);  printf("Break statement\n");
            break;
        case SKIP_STMT: 
            _putIndent(indent);  printf("Skip statement\n");
            break;
        case BLOCK_STMT: 
            _putIndent(indent);  printf("Block:\n");
            _debugStmtList(stmt->body._block->stmt_list, indent + 1);
            break;

        // Declaration
        case VAR_DECL: {
            char* var_name = getLexeme(stmt->body._var_decl->name);
            _putIndent(indent);  printf("Variable declaration statement:\n");
            _putIndent(indent + 1);  printf("Name: %s\n", var_name);
            free(var_name);
            break;
        }
    }
    return true;
}

static bool _debugExpr(Expr* expr, int indent){
    if(expr == NULL) {
         _putIndent(indent);  printf("Null expression\n");
         return false;
    }

    const char *op;

    switch(expr->type){
        case LITERAL_EXPR: {
            char *lexeme = getLexeme(expr->body._lit->token);
            _putIndent(indent);  printf("%s\n", lexeme);
            free(lexeme);
            break;
        }
        case BINARY_EXPR: 
            switch(expr->body._bin->op.type){
                case TOKEN_AND: op = "and"; break;
                case TOKEN_OR: op = "or"; break;
                case TOKEN_PLUS: op = "+"; break;
                case TOKEN_MINUS: op = "-"; break;
                case TOKEN_STAR: op = "*"; break;
                case TOKEN_SLASH: op = "/"; break;
                case TOKEN_EQUAL_EQUAL: op = "=="; break;
                case TOKEN_BANG_EQUAL: op = "!="; break;
                case TOKEN_GREATER: op = ">"; break;
                case TOKEN_GREATER_EQUAL: op = ">="; break;
                case TOKEN_LESS: op = "<"; break;
                case TOKEN_LESS_EQUAL: op = "<="; break;
            }
            _putIndent(indent);  printf("Binary %s\n", op);
            _debugExpr(expr->body._bin->left, indent+1);
            _debugExpr(expr->body._bin->right, indent+1);
            break;
        case GROUP_EXPR: 
            _putIndent(indent);  printf("Grouping\n", op);
            _debugExpr(expr->body._group->expr, indent+1);
            break;
        case UNARY_EXPR: 
            switch(expr->body._unary->op.type){
                case TOKEN_MINUS: op = "-"; break;
                case TOKEN_BANG: op = "!"; break;
            }
            _putIndent(indent);  printf("Unary %s\n", op);
            _debugExpr(expr->body._unary->right, indent+1);
            break;
        case VAR_EXPR: {
            char *var_name = getLexeme(expr->body._var->name);
            _putIndent(indent);  printf("Variable @%s\n", var_name);
            free(var_name);
            break;
        }
        case ASSIGNMENT_EXPR: 
            char *var_name = getLexeme(expr->body._assign->var);
            _putIndent(indent++); printf("Assignment\n");
            _putIndent(indent); printf("Asignee: \n");
            _putIndent(indent+1); printf("%s\n", var_name);
            _putIndent(indent); printf("Value: \n");
            _debugExpr(expr->body._assign->val, indent+1);
            free(var_name);
            break;
    }

    return true;
}