#include "compiler/token.h"
#include <stdlib.h>

char* getLexeme(Token token){
    int i;
    char* dest = malloc(token.length + 1);
    for(i = 0; i < token.length; i++) 
        dest[i] = *(token.start + i);
    dest[i] = '\0';
    return dest;
}

TokenList newTokenList(){
    TokenList list;
    list.count = 0;
    list.tokens = NULL;
    return list;
}

void appendTokenList(TokenList* list, Token token){
    list->tokens = realloc(list->tokens, sizeof(Token) * (list->count + 1));
    list->tokens[list->count] = token;
    list->count++;
}

void freeTokenList(TokenList* list){
    if(list->tokens != NULL) {
        free(list->tokens);
        list->tokens = NULL;
        list->count = 0;
    }
}