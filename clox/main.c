#include "common.h"
#include "chunk.h"
#include "debug.h"

#include <stdio.h>

int
main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 2, 1);
    writeConstant(&chunk, 1000, 2);

    disassembleChunk(&chunk, "Test chunk");
    
    freeChunk(&chunk);
    
    return 0;
}