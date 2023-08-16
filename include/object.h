#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "value.h"

// Returns object type tag of a given (obj) Value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Calls a function because value is called twice in the body,
// and macros insert the /expression/ every place the param appears
// in the body. That would mean the expr would get evaluated twice.
// That is bad if expr has side effects e.g., `IS_STRING(POP())`
// which would pop /two/ values off the stack.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Given a valid Obj Value, it is downcasted into a the appropriate ptr type.
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
// This is a convenience macro that steps through and returns the char array.
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

// Any lox value who's state lives on the heap
struct Obj {
  ObjType type;
};

struct ObjString {
  // Strings are Obj, and need to have state that all Obj's share.
  // This is done by having first field be an Obj.
  // NOTE: C specifies that struct fields are arranged in memory in the
  // order that they are declared & expanding inner struct fields is in place.
  Obj obj;
  int length;
  // String contains an array of chars. Stored in separate,
  // heap allocated array to set aside only as much room
  // as needed for each string.
  // Number of bytes is also stored in array for convenience to
  // know how much memory is allocated without walking whole char array
  // till null terminator.
  char *chars;
};

ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
void printObject(Value value);

// Checks if a given Value is an obj, of type `type`.
static inline bool isObjType(Value value, ObjType type) {
  // `&&` bails early
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif