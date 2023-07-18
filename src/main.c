#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Starts a REPL instance
// REPL ideally handles input that spans multiple lines
//  and doesn’t have a hardcoded line length limit.
static void repl() {
  char line[1024];
  while (1) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }

    interpret(line);
  }
}

// Reads a file, dynamically allocating it to a buffer
// and returns it passing ownership to its caller
static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  // Seek to end to "tell" how many bytes the file is
  // this is so we can allocate a buffer large enough
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  // Rewind back to the start of the file.
  rewind(file);

  // Allocate buffer to store file bytes +1 (for null-byte)
  char *buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

// Reads file and executes resulting string of Lox source code.
// Exits program on compile or runtime error
static void runFile(const char *path) {
  char *source = readFile(path);
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR)
    exit(65);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(70);
}

int main(int argc, const char *argv[]) {
  initVM();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  freeVM();
  return 0;
}