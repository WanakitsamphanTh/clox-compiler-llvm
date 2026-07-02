#include "compiler/scanner.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

Scanner scanner;


void initScanner(const char* source){
    scanner.start = scanner.current = source;
    scanner.line = 0;
}

Token scanToken(){
    Token token;
    int length = 1;
    char c;

    while(1){
        skipSpace();

        if(scannerMatchAndAdvance("#"))
            omitComment();
        else
            break;
    }

    scanner.start = scanner.current;
    token.start = scanner.start;
    token.line = scanner.line;
    c = scannerAdvance();
    switch(c){
        case '(':
            token.type = TOKEN_LEFT_PAREN; break;
        case ')':
            token.type = TOKEN_RIGHT_PAREN; break;
        case '{':
            token.type = TOKEN_LEFT_BRACE; break;
        case '}':
            token.type = TOKEN_RIGHT_BRACE; break;
        case ';':
            token.type = TOKEN_SEMICOLON; break;
        case '.':
            token.type = TOKEN_DOT; break;
        case ',':
            token.type = TOKEN_COMMA; break;
        case '+':
            token.type = TOKEN_PLUS; break;
        case '-':
            token.type = TOKEN_MINUS; break;
        case '*':
            token.type = TOKEN_STAR; break;
        case '/':
            token.type = TOKEN_SLASH; break;
        case '!':
            if(scannerMatchAndAdvance("=")) {
                token.type = TOKEN_BANG_EQUAL;
                length++;
            } else {
                token.type = TOKEN_BANG;
            }
            break;
        case '=':
            if(scannerMatchAndAdvance("=")) {
                token.type = TOKEN_EQUAL_EQUAL;
                length++;
            } else {
                token.type = TOKEN_EQUAL;
            }
            break;
        case '>':
            if(scannerMatchAndAdvance("=")) {
                token.type = TOKEN_GREATER_EQUAL;
                length++;
            } else {
                token.type = TOKEN_GREATER;
            }
            break;
        case '<':
            if(scannerMatchAndAdvance("=")) {
                token.type = TOKEN_LESS_EQUAL;
                length++;
            } else {
                token.type = TOKEN_LESS;
            }
            break;
        case '\"':
            token.type = scanString(&length); break;
        case '\0':
            token.type = TOKEN_EOF; break;
        default:
            if(isdigit(c)) {
                token.type = TOKEN_NUMBER;
                scanNumber(&length);
            }
            else if(isalpha(c) || c == '_') token.type = scanWord(&length);
            else token.type = TOKEN_ERROR;
    }

    token.length = length;
    return token;
}

void skipSpace(){
    while(1){
        switch(scannerPeek()){
            case '\n':
                scanner.line++;
            case ' ':
            case '\t':
            case '\r':
                scannerAdvance();
                break;
            default:
                return;
        }
    }
}

void omitComment(){
    char c;
    while (scannerPeek() != '\n' && scannerPeek() != '\0')
        scannerAdvance();
}

char scannerPeek(){
    return *scanner.current;
}

char scannerPeekNext(){
    return *(scanner.current + 1);
}

char scannerAdvance(){
    if(*scanner.current == '\0')
        return '\0';
    return *(scanner.current++);
}

int scannerMatchAndAdvance(const char* chars){
    while(*chars){
        if(scannerPeek() == *chars++){
            scannerAdvance();
            return 1;
        }
    }
    return 0;
}

void scanNumber(int* length){
    while(isdigit(scannerPeek())){
        scannerAdvance();
        (*length)++;
    }
    if(scannerMatchAndAdvance(".")){
        (*length)++;
        while(isdigit(scannerPeek())){
            scannerAdvance();
            (*length)++;
        }
    }
}

#define MATCH(word, keyword, token_type) if(strcmp(word, keyword) == 0) return token_type
TokenType scanWord(int* length){
    char word[32];
    int i;
    while(isalnum(scannerPeek()) || scannerPeek() == '_'){
        (*length)++;
        scannerAdvance();
    }

    for(i = 0; i < *length; i++)
        word[i] = scanner.start[i];
    word[i] = '\0';
    
    MATCH(word, "and", TOKEN_AND);
    MATCH(word, "class", TOKEN_CLASS);
    MATCH(word, "else", TOKEN_ELSE);
    MATCH(word, "false", TOKEN_FALSE);
    MATCH(word, "for", TOKEN_FOR);
    MATCH(word, "fun", TOKEN_FUN);
    MATCH(word, "if", TOKEN_IF);
    MATCH(word, "nil", TOKEN_NIL);
    MATCH(word, "or", TOKEN_OR);
    MATCH(word, "print", TOKEN_PRINT);
    MATCH(word, "return", TOKEN_RETURN);
    MATCH(word, "super", TOKEN_SUPER);
    MATCH(word, "this", TOKEN_THIS);
    MATCH(word, "true", TOKEN_TRUE);
    MATCH(word, "var", TOKEN_VAR);
    MATCH(word, "while", TOKEN_WHILE);
    MATCH(word, "skip", TOKEN_SKIP);
    MATCH(word, "break", TOKEN_BREAK);

    return TOKEN_IDENTIFIER;
}

static TokenType scanString(int* length){
    char c;
    while(1) {
        c = scannerAdvance();
        if(c == '\0') return TOKEN_ERROR;
        (*length)++;
        if(c == '\"') break;
        if(c == '\n') scanner.line++;
        if(c == '\\'){
            char next = scannerAdvance();
            if (next == '\0')
                return TOKEN_ERROR;
            (*length)++;
            if (next == '\n')
                scanner.line++;
        }
    }
    return TOKEN_STRING;
}