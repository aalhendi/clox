#include <stdio.h>
#include "debug.h"

// Disassembles all instructions in a chunk
void disassembleChunk(Chunk *chunk, const char *name)
{
    // Print a header with the chunk name
    printf("== %s ==\n", name);

    // Disassemble each instruction in bytecode array
    int offset = 0;
    while (offset < chunk->count)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

// Prints name of instruction, constant index and constant value
// Returns offset+2
static int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constantIdx = chunk->code[offset + 1];
    // Print name of instruction and constant index (from subsequent byte in chunk)
    printf("%-16s %4d '", name, constantIdx);
    // Print constant value. Constants are known at compile-time
    printValue(chunk->constants.values[constantIdx]);
    printf("'\n");

    return offset + 2;
}

// Prints offset of instruction and its name
// Returns new offset
int disassembleInstruction(Chunk *chunk, int offset)
{
    // Prints byte offset of given instruction
    printf("%04d ", offset);

    // Read single byte from bytecode array at given offset
    uint8_t instruction = chunk->code[offset];

    switch (instruction)
    {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

// Prints the name of the instruction
// Returns offset+1
static int simpleInstruction(const char *name, int offset)
{
    // Prints name of the instruction
    printf("%s\n", name);
    return offset + 1;
}