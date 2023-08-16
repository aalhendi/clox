#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

// Initializes a new dynamic value array
void initValueArray(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

// Appends a value to the end of the dynamic array
void writeValueArray(ValueArray *array, Value value) {
  if (array->capacity <= array->count) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values =
        GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

// Decallocates all array-related memory and zeros fields.
void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array); // Leaves dynamic array in a well-defined, empty state
}

// Prints a value
void printValue(Value value) {
  switch (value.type) {
  case VAL_BOOL: {
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  }
  case VAL_NIL: {
    printf("nil");
    break;
  }
  case VAL_NUMBER: {
    printf("%g", AS_NUMBER(value));
    break;
  }
  case VAL_OBJ: {
    printObject(value);
    break;
  }
  }
}

// Compares two lox values. If types differ then false,
// else unwrap values to C values and compare directly based on a's type.
// We dont use memcmp() because some values vary in padding.
bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
  case VAL_BOOL: {
    return AS_BOOL(a) == AS_BOOL(b);
  }
  case VAL_NIL: {
    return true;
  }
  case VAL_NUMBER: {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  case VAL_OBJ: {
    // Even if both string literals are equal, they won't have the same memory
    // address because each literal is allocated on the heap seperately and so
    // we need to compare the contents and not just the mem addr.
    // NOTE: Even if its the same object, we still memcmp the string. That means
    //       equality checks for strings are slower.
    ObjString *aString = AS_STRING(a);
    ObjString *bString = AS_STRING(b);
    return aString->length == bString->length &&
           memcmp(aString->chars, bString->chars, aString->length) == 0;
  }
  default:
    return false; // Unreachable
  }
}