#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct VM {
  Chunk *chunk;
  // NOTE: This would ideally be stored in a local variable so that the compiler
  // would keep it in a register. Instruction Pointer, Also called the Program
  // Counter (PC)
  uint8_t *ip;
  // LIFO, semantics implemented on top of a raw C-aray
  Value stack[STACK_MAX];
  // A pointer to the "top" of the stack. It is faster to dereference a pointer
  // than to calculate the offset when needed. It points to where next value is
  // to be pushed.
  Value *stackTop;
} VM;

typedef enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk *chunk);
static InterpretResult run();
static void resetStack();
void push(Value value);
Value pop();

#endif
