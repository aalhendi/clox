#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct Parser {
  Token current;
  Token previous;
  bool hadError;
  // C doesn't have exceptions which can unwind parser.
  // would be nice but a bool flag works too.
  // Used to skip tokens and resynchronize.
  bool panicMode;
} Parser;

typedef enum Precedence {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

// C's syntax for function ptrs is bad. So we wrap with a typedef.
typedef void (*ParseFn)();

typedef struct ParseRule {
  // function to compile a prefix expression starting with a token of that type.
  ParseFn prefix;
  // function to compile an infix expression whose left operand is followed by a
  // token of that type.
  ParseFn infix;
  // precedence of an infix expression that uses that token as an operator.
  Precedence precedence;
} ParseRule;

// Single global variable of parser struct.
// This is to save from passing state around from function to function.
Parser parser;
Chunk *compilingChunk;

// Returns a pointer to the current chunk being compiled.
static Chunk *currentChunk() { return compilingChunk; }

// Prints an error to standard error and sets hadError flag in parser.
static void errorAt(Token *token, const char *message) {
  // Simply surpress any other errors while in panic mode.
  // This is so we keep compiling as normal as if error never
  // occured. Keep on trucking. Bytecode wont be executed.
  // Parser might go off track but user won't ever know.
  // Panic mode ends when parser reacher resync point.
  if (parser.panicMode) {
    return;
  }
  parser.panicMode = true;
  // Print line information from token.
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (TOKEN_ERROR) {
    // Do nothing
  } else {
    // Print lexeme
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

// Pulls location info from previously consumer token and calls errorAt().
static void error(const char *message) { errorAt(&parser.previous, message); }

// Pulls location info from current token and forwards to errorAt().
// Called when scanner hands back an error token.
static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

// Steps forward through token stream,
// storing next token in current and current token in previous.
static void advance() {
  parser.previous = parser.current;

  while (true) {
    parser.current = scanToken();

    // Loop until end or non-error token
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    // since scanner doesn't report lexical errors and simply
    // creates tokens, error tokens are handled here in the parser/compiler
    errorAtCurrent(parser.current.start);
  }
}

// Wrapper around advance() while validating if token has expected
// type. If not, reports an error.
static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

// Appends a byte to the current chunk.
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

// Wrapper around emitByte().
// Appends 2 bytes. Useful for opcde + 1-byte operand cases.
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

// Emits a return instruction
static void emitReturn() { emitByte(OP_RETURN); }

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  // Overflow check
  // BONUS: Support OP_CONSTANT_16
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

// Emits a constant opode instruction and inserts an entry in the constants
// table.
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
// Dump the chunk if no parser errors.
// We could print dissasembly even with errors, since no bytecode is executed.
// BUT it would be pointless since the parser would be in a confused state.
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
  emitReturn();
}

// Forward declarations to keep compiler happy :)
static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// Assumes entire left hand operand expression has been compiled AND
// subsequent infix operator has been consumed and stored in previous.
// Fetches the appropriate rule for the operator's type and parses precedence.
// Emits the appropriate bytecode instruction that performs the binary
// operation.
static void binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_BANG_EQUAL: {
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  }
  case TOKEN_EQUAL_EQUAL: {
    emitByte(OP_EQUAL);
    break;
  }
  case TOKEN_GREATER: {
    emitByte(OP_GREATER);
    break;
  }
  // NOTE: IEEE 754 is NOT IMPLEMENTED.
  //  NaN <= 1 is false and NaN > 1 is also false. But our desugaring assumes
  //  the latter is always the negation of the former.
  case TOKEN_GREATER_EQUAL: {
    emitBytes(OP_LESS, OP_NOT);
    break;
  }
  case TOKEN_LESS: {
    emitByte(OP_LESS);
    break;
  }
  case TOKEN_LESS_EQUAL: {
    emitBytes(OP_GREATER, OP_NOT);
    break;
  }
  case TOKEN_PLUS: {
    emitByte(OP_ADD);
    break;
  }
  case TOKEN_MINUS: {
    emitByte(OP_SUBTRACT);
    break;
  }
  case TOKEN_STAR: {
    emitByte(OP_MULTIPLY);
    break;
  }
  case TOKEN_SLASH: {
    emitByte(OP_DIVIDE);
    break;
  }

  default:
    break; // Unreachable
  }
}

// Assumes keyword token already consumed by parsePrecedence()
// Returns instruction opcode based on TokenType
static void literal() {
  switch (parser.previous.type) {
  case TOKEN_FALSE: {
    emitByte(OP_FALSE);
    break;
  }
  case TOKEN_NIL: {
    emitByte(OP_NIL);
    break;
  }
  case TOKEN_TRUE: {
    emitByte(OP_TRUE);
    break;
  }
  default:
    return; // Unreachable
  }
}

// Reads next token and looks up corresponding prefix parse rule.
// Compiles prefix expression and consumes needed tokens.
// Then look for and compiles infix expression similarly.
static void parsePrecedence(Precedence precedence) {
  advance();
  // First token will /always/ belong to some prefix expression.
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Exprect expression.");
    return;
  }

  // Compiles prefix expression and consumes needed tokens.
  prefixRule();

  // Look for infix expression, prefix might be operand for it.
  // But only if precedence is of high enough precedence.
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

static void expression() {
  // Parse the lowest precedence level, which subsumes (includes) all of the
  // higher-precedence expressions too
  parsePrecedence(PREC_ASSIGNMENT);
}

// Assumes initial left paren token has been consumed.
// Recursively calls back into expression() to compile inner expr.
// Finally, consumes the closing right paren token.
static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// Assumes number token has been consumed and stored in previous.
// Converts number string lexeme to a value of type double.
// Finally, emits the constant.
static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

// Takes the string's characters directly from the lexeme.
// +1 and -2 trim the leading and trailing quotation marks.
// Then creates a string object, wraps it in a Value and emits
// it into the constants table.
// BONUS: Support escape sequences and translate them here e.g., ('\n')
static void string() {
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

// Assumes leading minus/bang token has been consumed and stored in previous.
// Recursively calls back into expression to compile operand.
// Emits bytecode to perform unary operation.
static void unary() {
  TokenType operatorType = parser.previous.type;

  // BONUS: Store line before compiling operand and pass into emitByte()
  // This would help with multi-line negation such as print - `\n` true;

  // Compile the operand
  parsePrecedence(PREC_UNARY);

  // Emit operator instruction
  switch (operatorType) {
  case TOKEN_BANG: {
    emitByte(OP_NOT);
    break;
  }
  case TOKEN_MINUS: {
    // emitted after operand bytecode because vm is stack based.
    emitByte(OP_NEGATE);
    break;
  }

  default:
    return; // Unreachable
  }
}

ParseRule rules[] = {
    // Uses the enum int representations to index the array.
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// Returns the rule at a given index. Rule is a function ptr
static ParseRule *getRule(TokenType type) { return &rules[type]; }

// Compiles the input source code to bytecode chunk
// Returns a boolean of success status
bool compile(const char *source, Chunk *chunk) {
  initScanner(source);
  compilingChunk = chunk; // Initialize compilingChunk ptr to input chunk.

  // Initialize parser flags
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  // Currently, only support expression parsing.
  // TODO: Add statements
  expression();
  // End of source code should always be denoted with an EOF token
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
