#ifndef CHUNK_H
#define CHUNK_H

#include <stddef.h>
#include "value.h"
#include "common.h"

typedef enum {
    OP_RETURN,
    OP_CONST,
    OP_NIL,

    // Boolean
    OP_TRUE,
    OP_FALSE,

    // Arithemtic operations
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MULT,
    OP_DIV,
    OP_MOD,

    // Boolean algebra and comparison
    OP_AND,
    OP_OR,
    OP_CMPL,
    OP_LESS,
    OP_LESS_EQ,
    OP_GREATER,
    OP_GREATER_EQ,
    OP_EQ,

    // Array
    OP_ARR,

    // control
    OP_JMP,
    OP_JIF, // jump if false
    OP_LOOP,

    OP_POP,
    OP_DEFINE_GLOB,
    OP_LOAD_GLOB,
    OP_STORE_GLOB,

    OP_LOAD_LOC,
    OP_STORE_LOC,

    OP_CALL,

    OP_PRINT
} OpCode;

typedef struct _Chunk {
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