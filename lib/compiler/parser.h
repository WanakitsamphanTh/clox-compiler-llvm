#ifndef PARSER_H
#define PARSER_H

#include "compiler/token.h"
#include "common.h"

typedef struct {
    Token  current;
    Token  previous;
    bool hasError;
} Parser;

extern Parser parser;

#endif