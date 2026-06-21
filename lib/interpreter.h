#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "vm.h"
#include "common.h"

char* readFile(const char* file_name);
InterpretResult repl();
InterpretResult interpretFile(const char* file_name);

#endif