#include <stdio.h>
#include "memory.h"
#include "value.h"

// Initializes a new dynamic value array
void initValueArray(ValueArray *array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

// Appends a value to the end of the dynamic array
void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity <= array->count)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// Decallocates all array-related memory and zeros fields.
void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array); // Leaves dynamic array in a well-defined, empty state
}

// Prints a value
void printValue(Value value)
{
    printf("%g", value);
}