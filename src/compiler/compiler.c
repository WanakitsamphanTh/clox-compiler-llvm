#include "compiler/compiler.h"
#include "compiler/scanner.h"
#include "compiler/token.h"
#include "compiler/parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

bool compile(const char* source, Chunk* chunk){
    int line = -1;
    char *lexeme;
    int i;
    Token token;
    StmtList statements;
    TokenList token_list = newTokenList();
    bool successful = true;

    // Scan and tokenization
    fprintf(stderr, "Start scanning\n");
    initScanner(source);

    while(1){
        token = scanToken();
        if(token.type == TOKEN_ERROR) {
            successful = false;
            goto compilation_terminated;
        }
        appendTokenList(&token_list, token);
        if(token.type == TOKEN_EOF) break;
    }

    for(i = 0; i < token_list.count; i++){
        token = *(token_list.tokens + i);
        if(token.type == TOKEN_EOF) {
            printf("EOF\n");
        } else {
            lexeme = getLexeme(token);
            printf("Token type: %2d line: %d lexeme: %s\n", token.type, token.line, lexeme);
            free(lexeme);
        }
    }

    // Parsing and AST generation
    initParser(token_list.tokens);
    statements = parse();

    if(parser.has_error) {
        fprintf(stderr, "Parser error: %s", parser.err_msg);
        successful = false;
        goto compilation_terminated;
    }

    // Compilation
    writeChunk(chunk, OP_RETURN);

    //Free memory on termination
    compilation_terminated:
        freeStmtList(&statements); 
        freeTokenList(&token_list);

    return successful;   
}