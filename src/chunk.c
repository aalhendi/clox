#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

// Initializes a new chunk
void initChunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
}

// Appends a byte to the end of a chunk
void writeChunk(Chunk *chunk, uint8_t byte)
{
    if (chunk->capacity <= chunk->count)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

// Decallocates all chunk-related memory and zeros fields.
void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    initChunk(chunk); // Leaves chunk in a well-defined, empty state
}