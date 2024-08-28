#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

#include <stdio.h>

int
main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    initVM();

    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 1.2, 1);
    writeConstant(&chunk, 3.4, 1);
    writeChunk(&chunk, OP_ADD, 1);
    writeConstant(&chunk, 5.6, 1);
    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 123);

    interpret(&chunk);

    freeChunk(&chunk);

    freeVM();

    return 0;
}