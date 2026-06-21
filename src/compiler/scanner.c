#include "compiler/scanner.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

Scanner scanner;


void initScanner(const char* source){
    scanner.start = scanner.current = source;
    scanner.line = 0;
}

Token scanToken(){
    Token token;
    int length = 1;
    token.start = scanner.current;
    token.line = scanner.line;
    char c = scannerAdvance();

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
            if(scannerPeekNext() == '=') {
                token.type = TOKEN_BANG_EQUAL;
                length++;
            } else {
                token.type = TOKEN_BANG;
            }
            break;
        case '=':
            if(scannerPeekNext() == '=') {
                token.type = TOKEN_EQUAL_EQUAL;
                length++;
            } else {
                token.type = TOKEN_EQUAL;
            }
            break;
        case '>':
            if(scannerPeekNext() == '=') {
                token.type = TOKEN_GREATER_EQUAL;
                length++;
            } else {
                token.type = TOKEN_GREATER;
            }
            break;
        case '<':
            if(scannerPeekNext() == '=') {
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
    while(scannerPeek() == ' ' || scannerPeek() == '\t'){
        scanner.current++;
    }
    while(scannerPeek() == '\n') {
        scanner.current++;
        scanner.line++;
    }
}

void omitComment(){
    if(scannerPeek() == '#') while(scannerAdvance() != '\n');
}

char scannerPeek(){
    return *scanner.current;
}

char scannerPeekNext(){
    return *(scanner.start + 1);
}

char scannerAdvance(){
    return *(scanner.current++);
}

int scannerMatchAndAdvance(const char* chars){
    char c = scannerPeek();
    while(*chars != '\0'){
        if(c == *chars){
            scannerAdvance();
            return 1;
        }
        chars++;
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
        word[i] = *(scanner.current + i);
    
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
            scannerAdvance();
            (*length)++;
        }
    }
    return TOKEN_STRING;
}