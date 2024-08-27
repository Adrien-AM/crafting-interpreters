#include "debug.h"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
simpleInstruction(const char name[], int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int
constantInstruction(const char name[], Chunk* chunk, int offset, int operand_size)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-4s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 1 + operand_size;
}

void
disassembleChunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

int
disassembleInstruction(Chunk* chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && getLine(chunk, offset) == getLine(chunk, offset - 1)) {
        printf(" | ");
    } else {
        printf("%4d ", getLine(chunk, offset));
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset, 1);
        case OP_CONSTANT_LONG:
            return constantInstruction("OP_CONSTANT_LONG", chunk, offset, 3);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
