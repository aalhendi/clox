#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// Calculates new capacity based on given current capacity.
// Returns 8 if capacity < 8 else capacity * 2
#define GROW_CAPACITY(capacity) ((capacity < 8) ? 8 : capacity * 2)

// Wrapper arround `reallocate()` function
// Gets size of array element type to calculate total oldSize and newSize
// Casts the void* return type to type*
#define GROW_ARRAY(type, pointer, oldCount, newCount) (type *)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

// Wrapper around `reallocate()` funciton
// Passes 0 for newSize.
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif