#ifndef CHUNK_H
#define CHUNK_H

#include <stddef.h>
#include "value.h"

typedef unsigned char uint8_t;

typedef enum {
    OP_RETURN,
    OP_CONST,

    // Arithemtic operations
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MULT,
    OP_DIV,

    OP_LOAD,
    OP_STORE,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    ValueArray constants;
} Chunk;

Chunk newChunk();
void writeChunk(Chunk* chunk, uint8_t code);
void freeChunk(Chunk* chunk);
void disassembleChunk(const Chunk* chunkm, const char* chunk_name);
int disassembleInstruction(const Chunk* chunk, int offset);
int addConstant(Chunk* chunk, Value value);

#endif