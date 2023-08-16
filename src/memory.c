#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

/*
Wrapper arround built in `realloc()`
newSize == 0:
    frees memory at pointer and returns null
oldSize == 0:
    will malloc, allocating a new block of memory
newSize < oldSize:
    update block size, return old pointer
    memory allocator keeps info on heap allocated blocks including size
newSize > oldSize:
    attempts to grow existing block if memory block after isnt in use
    else allocates new block, copies bytes over, frees old block
    and returns ptr to new block
*/
void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, newSize);

  // `realloc()` can fail if if there is not enough memory
  if (result == NULL)
    exit(1);

  return result;
}

// Frees an Object Value based on its type
static void freeObject(Obj *object) {
  switch (object->type) {
  case OBJ_STRING: {
    ObjString *string = (ObjString *)object;
    // Free the character array stored in a string Object.
    FREE_ARRAY(char, string->chars, string->length + 1);
    // Free the actual Object struct.
    FREE(ObjString, object);
    break;
  }
  }
}

// Frees all objects
void freeObjects() {
  Obj *object = vm.objects;
  // while the head ptr points to an object
  while (object != NULL) {
    // Free the object, update the ptr.
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }
}