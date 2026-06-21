#include "compiler/compiler.h"
#include "compiler/scanner.h"
#include "compiler/token.h"
#include "vm.h"
#include <stdio.h>

bool compile(const char* source, Chunk* chunk){
    int line = -1;
    char lexeme[128];
    int i;

    fprintf(stderr, "Start scanning\n");

    initScanner(source);

    while(1){
        skipSpace();
        omitComment();
        Token token = scanToken();
        if(token.type == TOKEN_ERROR) {
            printf("Scan error at line %2d", token.line);
            break;
        }
        if(token.type == TOKEN_EOF) break;
    }

    return true;   
}