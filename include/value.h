#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// Struct inheritance: roughly follows how single-inheritance
// of state works in object-oriented languages.
// Declared here because of some cyclic dependencies between values and objects.
// Actual definition is in `object.h`
typedef struct Obj Obj;
typedef struct ObjString ObjString;

// The VM's notion of a type, not the user's
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ, 
} ValueType;

// Assuming x64 Values are 16-bytes
typedef struct Value {
  ValueType type; // 4-bytes
  // 4-bytes padding to keep close to nearest 8-byte boundary
  // 8-byte union
  union as {
    bool boolean;  // 1-byte
    double number; // 8-bytes
    Obj *obj;      // 4-bytes
  } as;
} Value;

// Used to guard AS_ macro calls. Return booleans.
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// UNSAFE: Unwrap Value and return the corresponding raw C value.
#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
// NIL has one value and no data, we dont need a cast to it.

// Cast to a Value type and define struct fields, {type, {.unionField}}
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
// Takes a bare object ptr and wraps it in a value
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj *)object}})

// Dynamic array
typedef struct ValueArray {
  int capacity;
  int count;
  Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif