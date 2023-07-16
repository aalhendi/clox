#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include <stdio.h>

VM vm;

static void resetStack() {
  // Since stack won't be used till values are stored inside
  // there is no need to allocate it or clear it
  // Set stackTop ptr to point to beginning of stack to indicate its empty
  vm.stackTop = vm.stack;
}

// Initializes the VM
void initVM() { resetStack(); }

void freeVM() {}

// Appends a value to the end of the stack and increments the stackTop pointer
void push(Value value) {
  // Stores value in the address pointed to by the stackTop pointer
  *vm.stackTop = value;
  // Increment the pointer
  vm.stackTop++;
}

// "Removes" the last value from the stack
// returning it and decrementing the stack pointer
Value pop() {
  // Decrement the pointer.
  vm.stackTop--;
  // stackTop now points to last value in stack, return it
  // no need to explicitly remove, it will be overwritten
  return *vm.stackTop;
}

InterpretResult interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}

// Handles decoding or dispatching the instruction.
static InterpretResult run() {
// Reads byte currently pointed at by IP then advances IP
#define READ_BYTE() (*vm.ip++)
// Reads next byte from bytecode using it as index into chunk constants
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    // Print every value in the stack from bottom to top
    // start at initial addr of stack, stop at last addr as marked by stackTop
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    // Since current instruction reference is stored as direct pointer
    // We must convert IP back to relative offset from begining of bytecode
    // Then disassemble instruction beginning at that byte
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction = READ_BYTE();
    // NOTE: This isn't the fastest way to handle bytecode dispatch
    // (see: Computed goto, jump table, direct threaded code)
    // switch-case is used here because it is simple and within standard library
    switch (instruction) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}