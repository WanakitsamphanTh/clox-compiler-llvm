#include "chunk.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>   
#include "memory.h"

static int simpleInstruction(const char*, int);
static int jumpInstruction(const char*, int, const Chunk*, int);
static int constantInstruction(const char*, const Chunk*, int);
static int arrayInstruction(const char*, const Chunk*, int);
static int arrayInitInstruction(const char*, const Chunk*, int);
static int callInstruction(const char*, const Chunk*, int);
static int localInstruction(const char*, const Chunk*, int);
static int closureInstruction(const char*, const Chunk*, int);


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
        case OP_MOD:
            return simpleInstruction("OP_MOD", offset);

        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_ARR:
            return arrayInstruction("OP_ARR", chunk, offset);
        case OP_COLLECT:
            return arrayInitInstruction("OP_COLLECT", chunk, offset);
           
        case OP_AND: 
            return simpleInstruction("OP_AND", offset);
        case OP_OR:
            return simpleInstruction("OP_OR", offset);
        case OP_CMPL:
            return simpleInstruction("OP_CMPL", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_LESS_EQ:
            return simpleInstruction("OP_LESS_EQ", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_GREATER_EQ:
            return simpleInstruction("OP_GREATER_EQ", offset);
        case OP_EQ:
            return simpleInstruction("OP_EQ", offset);

        case OP_POP:
            return simpleInstruction("OP_POP", offset);

        case OP_DEFINE_GLOB:
            return constantInstruction("OP_DEFINE_GLOB", chunk, offset);
        case OP_LOAD_GLOB:
            return constantInstruction("OP_LOAD_GLOB", chunk, offset);
        case OP_STORE_GLOB:
            return constantInstruction("OP_STORE_GLOB", chunk, offset);
        case OP_LOAD_LOC:
            return localInstruction("OP_LOAD_LOC", chunk, offset);
        case OP_STORE_LOC:
            return localInstruction("OP_STORE_LOC", chunk, offset);
        case OP_LOAD_UVAL:
            return localInstruction("OP_LOAD_UVAL", chunk, offset);
        case OP_STORE_UVAL:
            return localInstruction("OP_STORE_UVAL", chunk, offset);
        case OP_CLOSE_UVAL:
            return localInstruction("OP_CLOSE_UVAL", chunk, offset);
        case OP_CLOSURE:
            return closureInstruction("OP_CLOSURE", chunk, offset);

        case OP_CONST:
            return constantInstruction("OP_CONST", chunk, offset);

        case OP_JMP:
            return jumpInstruction("OP_JMP", 1, chunk, offset);
        
        case OP_JIF:
            return jumpInstruction("OP_JIF", 1, chunk, offset);
        
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
            
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);

        case OP_CALL:
            return callInstruction("OP_CALL", chunk, offset);
        case OP_GET_IND:
            return simpleInstruction("OP_GET_IND", offset);
        case OP_SET_IND:
            return simpleInstruction("OP_SET_IND", offset);
    
        default:
            fprintf(stderr, "unknown opcode %d\n", op);
            return offset + 1;
    }
}


int addConstant(Chunk* chunk, Value value){
    int i;
    for(i = 0; i < chunk->constants.count; i++)
        if(compareValue(value, chunk->constants.values[i])) 
            return i;
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int simpleInstruction(const char* instruction, int offset) {
    printf("%s\n", instruction);
    return offset + 1;
}

int constantInstruction(const char* instruction, const Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    Value value = chunk->constants.values[constant];
    char buffer[256];
    memset(buffer,0,256);
    valueToString(value, buffer, 255);
    printf("%-16s %04d ", instruction, constant);
    if(value.type == OBJ_VALUE){
        if(value.val.obj->type == OBJ_STRING){
            char tmp[256];
            encodeString(tmp, buffer);
            printf("\'%s\'", tmp);
        }
        else {
            printf("<Object>");
        }
    } else printf("\'%s\'", buffer);
    printf("\n");
    return offset + 2;
}

int localInstruction(const char* instruction, const Chunk* chunk, int offset){
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %04d\n", instruction, slot);
    return offset + 2;
}

int jumpInstruction(const char* instruction, int sgn, const Chunk* chunk, int offset){
    uint16_t jmp = (uint16_t)(chunk->code[offset + 1] << 8) | chunk->code[offset + 2] & 0xffff;
    printf("%-16s %+04d -> %04d\n", instruction, sgn * jmp, offset + 3 + sgn * jmp);
    return offset + 3;
}

int arrayInstruction(const char* instruction, const Chunk* chunk, int offset){
    printf("%-16s %04d\n", instruction, chunk->code[offset + 1]);
    return offset + 2;
}

int arrayInitInstruction(const char* instruction, const Chunk* chunk, int offset){
    printf("%-16s %04d %04d\n", instruction, chunk->code[offset + 1], chunk->code[offset + 2]);
    return offset + 3;
}

static int callInstruction(const char* instruction, const Chunk* chunk, int offset){
    printf("%-16s %04d\n", instruction, chunk->code[offset + 1]);
    return offset + 2;
}

typedef enum {
    UVAL_LOC,
    UVAL_UVAL
} UpValueType;
const char* upvalue_type[] = {"UVAL_LOC", "UVAL_UVAL"};

int closureInstruction(const char* instruction, const Chunk* chunk, int offset){
    uint8_t fn_constant = chunk->code[++offset];
    uint8_t count = chunk->code[++offset];
    printf("%-16s %04d %02d\n", instruction, fn_constant, count);
    while(count > 0){
        UpValueType type = chunk->code[++offset];
        uint8_t slot = chunk->code[++offset];
        printf("     %s %d\n", upvalue_type[type], slot);
        count--;
    }
    return ++offset;
}