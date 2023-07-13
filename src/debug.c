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
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

// Prints the name of the instruction and returns offset+1
static int simpleInstruction(const char *name, int offset)
{
    // Prints name of the instruction
    printf("%s\n", name);
    return offset + 1;
}