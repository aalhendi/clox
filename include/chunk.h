#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_RETURN,
} OpCode;

typedef struct Chunk {
  // Number of allocated elements in use
  int count;
  // Number of elements allocated
  int capacity;
  // Pointer to dynamic array holding bytecode instructions
  uint8_t *code;
  // A dynamic value array to store chunk's constants
  ValueArray constants;
  // Another array to keep track of line numbers
  // BONUS: Impl a more efficient way of tracking lines.
  int *lines;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);

#endif