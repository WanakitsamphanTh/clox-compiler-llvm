#ifndef SCANNER_H
#define SCANNER_H

#include "compiler/token.h"

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

extern Scanner scanner;

void initScanner(const char*);
Token scanToken();
void skipSpace();
void omitComment();
static char scannerPeek();
static char scannerPeekNext();
static char scannerAdvance();
static int scannerMatchAndAdvance(const char* chars);
static void scanNumber(int* length);
static TokenType scanWord(int* length);
static TokenType scanString(int* length);

#endif