#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/_types/_va_list.h>

VM vm;

// Returns a value some distance from the top of the stack without popping it.
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

// Returns a bool of true if False or nil, else true.
// Lox borrows from Ruby. Only False and nil are falsey. 0 is true.
static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// Set stackTop ptr to point to beginning of stack to indicate its empty
static void resetStack() {
  // Since stack won't be used till values are stored inside
  // there is no need to allocate it or clear it
  vm.stackTop = vm.stack;
}

// Prints an runtime error to stderr
static void runtimeError(const char *format, ...) {
  // allows fn to be variadic, passing arbitrary number of arguments.
  va_list args;
  va_start(args, format);
  // fprintf variant that accepts an explicit va_list.
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // Get index of instruction in chunk - 1
  // because ip advances past instruction before executing it
  size_t instruction = vm.ip - vm.chunk->code - 1;
  // Look into chunk's debug line array.
  int line = vm.chunk->lines[instruction];
  // BONUS: Stack trace... when there's a call stack to trace.
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

// Handles decoding or dispatching the instruction.
static InterpretResult run() {
// Reads byte currently pointed at by IP then advances IP
#define READ_BYTE() (*vm.ip++)
// Reads next byte from bytecode using it as index into chunk constants
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// Binary ops only differ in the actual operator they use.
// This abstracts the boilerplate of shared for binary operations.
// do-while used to expand multi-statement macro with semicolon at the end.
// Check that both operands are numbers else throws a runtime error.
// If numbers, pops and unwraps Value types to doubles and executes operator
// and re-wraps \using valueType\ passed in before pushing back to the stack.
// NOTE: Pretty big macro... not neccecarily good C practice
#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false)

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
    case OP_NIL: {
      push(NIL_VAL);
      break;
    }
    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }
    case OP_FALSE: {
      push(BOOL_VAL(false));
      break;
    }
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }
    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_ADD: {
      BINARY_OP(NUMBER_VAL, +);
      break;
    }
    case OP_SUBTRACT: {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY: {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE: {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_NOT: {
      // isFalsey would return true for false -> inverting it
      push(BOOL_VAL(isFalsey(pop())));
      break;
    }
    case OP_NEGATE: {
      // Pops the top value in the stack, negates it and replaces it back
      // BONUS: might be faster to simply negate the value in place without
      // messing with stackTop
      // Peek top of stack and check if its a number.
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
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
#undef BINARY_OP
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

// Compiler the input source string into bytecode.
// Creates an empty chunk and passes it to the compiler.
// If compile success, sets vm bytecode chunk to compile result.
// Returns an InterpretResult
InterpretResult interpret(const char *source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();

  freeChunk(&chunk);
  return result;
}
