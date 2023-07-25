#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct Scanner {
  // Ptr to start of lexeme
  const char *start;
  // Ptr to current (to be consumed) char in lexeme
  const char *current;
  // current line number
  int line;
} Scanner;

Scanner scanner;

// Initializes a scanner instance.
void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

// Checks if input char is within ASCII a-z || A-Z || _
static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Checks if input char is within ASCII range 0-9
static bool isDigit(char c) { return c > '0' && c < '9'; }

// Checks if scanner's current ptr is at '\0'
static bool isAtEnd() { return *scanner.current == '\0'; }

// Produces a token given the type and current scanner ptrs
static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

// Advances the scanner's current ptr and returns character.
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

// Returns the character pointed to by the scanner's current ptr
static char peek() { return *scanner.current; }

// Returns the current+1 char pointed to by the scanner's current ptr
static char peekNext() {
  if (isAtEnd()) {
    return '\0';
  }
  return scanner.current[1];
}

// Compares the current value pointed at by the scanner's current ptr
// with the expected char input. Returns a boolean, false if EOF.
// If true, advances scanner's current ptr.
static bool match(char expected) {
  if (isAtEnd()) {
    return false;
  }
  if (*scanner.current != expected) {
    return false;
  }
  scanner.current++;
  return true;
}

// Produces an error token with ptrs to message string.
// This function is only called with string literals, which
// which are constant and have a long enough lifetime.
static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

// Peeks the current character and consumes it if whitespace, else breaks
static void skipWhitespace() {
  while (true) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t': {
      advance();
      break;
    }
    case '\n': {
      scanner.line++;
      advance();
      break;
    }
    case '/': {
      // BONUS: Support multi-line comments
      // Is a single-line comment
      if (peekNext() == '/') {
        // Discard till end of line
        while (peek() != '\n' && !isAtEnd()) {
          advance();
        }
      } else {
        return;
      }
      break;
    }
    default:
      return;
    }
  }
}

// Checks if lexeme is as long as keyword && remaining characters match exactly.
// Returns keyword token if matching else identifier token
static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

// A finite state machine to check if an identifier is a reserved keyword.
// Returns the appropriate token type
static TokenType identifierType() {
  switch (scanner.start[0]) {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_AND);
  case 'c':
    return checkKeyword(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return checkKeyword(1, 3, "lse", TOKEN_ELSE);
  case 'f': {
    // Check if there is a second letter in the lexeme
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return checkKeyword(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  }
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return checkKeyword(1, 2, "il", TOKEN_NIL);
  case 'o':
    return checkKeyword(1, 1, "r", TOKEN_OR);
  case 'p':
    return checkKeyword(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return checkKeyword(1, 4, "uper", TOKEN_SUPER);
  case 't': {
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return checkKeyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  }
  case 'v':
    return checkKeyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

// Consumes an ASCII alphanumeric identifier.
// Returns keyword token if exists else an identifier token.
static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) {
    advance();
  }

  return makeToken(identifierType());
}

// Consumes an ASCII number and returns a number token
static Token number() {
  // Consume number
  while (isDigit(peek())) {
    advance();
  }

  // Check for fractional part
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume '.'
    advance();

    // Consume fractional numbers
    while (isDigit(peek())) {
      advance();
    }
  }

  return makeToken(TOKEN_NUMBER);
}

// Consumes a string till '"' or EOF.
// Returns string token
static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    // Lox supports multi-line strings
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string.");
  }

  // consume closing quote
  advance();

  return makeToken(TOKEN_STRING);
}

// Scans a token and returns it by value
Token scanToken() {
  skipWhitespace();

  // Move start ptr to current position.
  scanner.start = scanner.current;

  if (isAtEnd()) {
    return makeToken(TOKEN_EOF);
  }

  char c = advance();
  if (isAlpha(c)) {
    return identifier();
  }
  if (isDigit(c)) {
    return number();
  }

  switch (c) {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '-':
    return makeToken(TOKEN_MINUS);
  case '+':
    return makeToken(TOKEN_PLUS);
  case '/':
    return makeToken(TOKEN_SLASH);
  case '*':
    return makeToken(TOKEN_STAR);
  case '!':
    return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }

  // Character does not belong to any known tokens
  return errorToken("Unexpected character.");
}