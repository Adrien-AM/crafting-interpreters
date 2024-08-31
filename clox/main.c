#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line, false);
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
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

static void runFile(const char* path, bool saveChunk) {
    char* source = readFile(path);
    InterpretResult result = interpret(source, saveChunk);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

static void runChunkFile(const char* path) {
    Chunk* chunk = readChunkFromFile(path);
    if (chunk == NULL) {
        fprintf(stderr, "Could not read chunk file \"%s\".\n", path);
        exit(74);
    }
    vm.chunk = chunk;
    vm.ip = chunk->code;
    InterpretResult result = run();
    freeChunk(chunk);
    free(chunk);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();
    if (argc == 1) {
        repl();
    } else if (argc == 2 || (argc == 3 && (strcmp(argv[1], "--save") == 0 || strcmp(argv[1], "--load") == 0))) {
        if (argc == 3 && strcmp(argv[1], "--save") == 0) {
            runFile(argv[2], true);
        } else if (argc == 3 && strcmp(argv[1], "--load") == 0) {
            runChunkFile(argv[2]);
        } else {
            runFile(argv[1], false);
        }
    } else {
        fprintf(stderr, "Usage: clox [--save | --load] [path]\n");
        exit(64);
    }
    freeVM();
    return 0;
}