#include "debug.h"
#include "chunk.h"
#include <stdio.h>

// Disassembles all instructions in a chunk
void disassembleChunk(Chunk *chunk, const char *name) {
  // Print a header with the chunk name
  printf("== %s ==\n", name);

  // Disassemble each instruction in bytecode array
  int offset = 0;
  while (offset < chunk->count) {
    offset = disassembleInstruction(chunk, offset);
  }
}

// Prints name of instruction, constant index and constant value
// Returns offset+2
static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constantIdx = chunk->code[offset + 1];
  // Print name of instruction and constant index (from subsequent byte in
  // chunk)
  printf("%-16s %4d '", name, constantIdx);
  // Print constant value. Constants are known at compile-time
  printValue(chunk->constants.values[constantIdx]);
  printf("'\n");

  return offset + 2;
}

// Prints the name of the instruction
// Returns offset+1
static int simpleInstruction(const char *name, int offset) {
  // Prints name of the instruction
  printf("%s\n", name);
  return offset + 1;
}

// Prints offset of instruction and its name
// Returns new offset
int disassembleInstruction(Chunk *chunk, int offset) {
  // Prints byte offset of given instruction
  printf("%04d ", offset);

  // Prints line number
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    // If instruction has same line number as previous, print `|`
    printf("   | ");
  } else {
    // Print the line number
    printf("%4d ", chunk->lines[offset]);
  }

  // Read single byte from bytecode array at given offset
  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_NIL:
    return simpleInstruction("OP_NIL", offset);
  case OP_TRUE:
    return simpleInstruction("OP_TRUE", offset);
  case OP_FALSE:
    return simpleInstruction("OP_FALSE", offset);
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_NOT:
    return simpleInstruction("OP_NOT", offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
