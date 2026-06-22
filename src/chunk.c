#include "chunk.h"
#include <stddef.h>
#include <stdio.h>
#include "memory.h"

static int simpleInstruction(const char*, int);
static int constantInstruction(const char*, const Chunk*, int);

Chunk newChunk() {
    Chunk chunk = {0,0,NULL, newValueArray()};
    return chunk;
}

void writeChunk(Chunk* chunk, uint8_t byte){
    int old_capacity;

    if(chunk->capacity < chunk->count + 1) {
        old_capacity = chunk->capacity;
        chunk->capacity = growCapacity(old_capacity);
        chunk->code = growArray(sizeof(uint8_t), chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void freeChunk(Chunk* chunk){
    chunk->code = freeArray(sizeof(uint8_t), chunk->code, chunk->capacity); 
    freeValueArray(&chunk->constants);
    *chunk = newChunk();
}

void disassembleChunk(const Chunk* chunk, const char* chunk_name) {
    int offset = 0;
    uint8_t op;

    printf("=======%s========\n", chunk_name);

    while(offset < chunk->count)
        offset = disassembleInstruction(chunk, offset);
}

int disassembleInstruction(const Chunk* chunk, int offset){
    printf("%04d ", offset);
    uint8_t op = chunk->code[offset];

    switch(op) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUB:
            return simpleInstruction("OP_SUB", offset);
        case OP_MULT:
            return simpleInstruction("OP_MULT", offset);
        case OP_DIV:
            return simpleInstruction("OP_DIV", offset);
        case OP_CONST:
            return constantInstruction("OP_CONST", chunk, offset);
        default:
            fprintf(stderr, "unknown opcode %d\n", op);
            return offset + 1;
    }
}

int addConstant(Chunk* chunk, Value value){
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int simpleInstruction(const char* instruction, int offset) {
    printf("%s\n", instruction);
    return offset + 1;
}

int constantInstruction(const char* instruction, const Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d\t", instruction, constant);
    printf("\'%.8f\'", chunk->constants.values[constant]);
    printf("\n");
    return offset + 2;
}