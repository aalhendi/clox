#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum
{
    OP_RETURN,
} OpCode;

typedef struct Chunk
{
    // Number of allocated elements in use
    int count;
    // Number of elements allocated
    int capacity;
    // Pointer to dynamic array holding bytecode instructions
    uint8_t *code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif