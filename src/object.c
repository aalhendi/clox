#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// Exists mainly to avoid the need to redundantly cast a void* back to the
// desired type
#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

// Allocates an Object of given size to the heap.
// Initializes the object's state.
// NOTE: size also includes extra bytes for payload fields necessary.
static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;

  return object;
}

// Creats a new ObjString on the heap and initializes its fields.
// Sort of like an initializer method in OOP langs.
static ObjString *allocateString(char *chars, int length) {
  // Creates the "base class" intializer to create an Object.
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

// Creates and allocates a null-terminated string on the heap
// via copying characters from an existing source.
ObjString *copyString(const char *chars, int length) {
  // Allocate a new array on the heap
  char *heapChars = ALLOCATE(char, length + 1);
  // Copy the chars to the fresh array.
  // NOTE: Even string literals are copied to the heap preemptively.
  memcpy(heapChars, chars, length);
  // Manually terminate string because lexeme points at range of
  // chars within source string monolith, and thats not null terminated.
  // We /could/ leave it unterminated because the length is known in ObjString.
  // BUT some C std library functions expect null terminated strings.
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}