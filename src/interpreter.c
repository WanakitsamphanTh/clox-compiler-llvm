#include "interpreter.h"
#include "vm.h"
#include "compiler/compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static InterpretResult interpret(VM*,const char*);

char* readFile(const char* file_name) {
    char* buffer;
    size_t bytes_read, file_size;
    FILE *file = fopen(file_name, "rb");
    if(!file) return NULL;

    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    buffer = malloc(file_size + 1);
    bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

InterpretResult repl(VM* vm) {
  char line[1024];
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\"%s\" \n", line);
      break;
    }

    if(strcmp(line,"\n") == 0) return INTERPRET_OK;
    interpret(vm, line);
  }
}

InterpretResult interpretFile(VM* vm, const char* file_name) {
    char* source = readFile(file_name);
    if(source == NULL) {
        fprintf(stderr, "Cannot open file %s", file_name);
        exit(-1);
    }
    interpret(vm, source);
    free(source);
    return INTERPRET_OK;
}

InterpretResult interpret(VM* vm, const char* source){
    Chunk chunk = newChunk();
    if(!compile(source, &chunk, &vm->heap)) {
        freeChunk(&chunk);
        return INTERPRET_ERROR;
    };
    //exit(-1);

    //vm.chunk = &chunk;
    //vm.ip = vm.chunk->code;

    return vmInterpret(vm, &chunk);
}
