#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "vm.h"
#include "common.h"

char* readFile(const char* file_name);
InterpretResult repl(VM* vm);
InterpretResult interpretFile(VM* vm, const char* file_name);

#endif