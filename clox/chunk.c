#include "chunk.h"
#include <stdlib.h>

void
initChunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->maxLines = 0;
    chunk->currentLine = 0;
    initValueArray(&chunk->constants);
}

int
getLine(Chunk* chunk, int index)
{
    int i, acc;
    i = acc = 0;
    while (1) {
        acc += chunk->lines[i + 1];
        if (acc > index)
            return chunk->lines[i];
        i += 2;
    }

    return -1;
}

/**
 * @brief Ensures the chunk has enough capacity to store a given number of bytes.
 * 
 * This function checks if the current capacity of the chunk is less than the required capacity.
 * If so, it grows the capacity of the chunk and reallocates the code array.
 *
 * @param chunk Pointer to the Chunk to ensure capacity for.
 * @param capacity The required capacity.
 */
void ensureCapacity(Chunk* chunk, int capacity)
{
    if (chunk->capacity < capacity) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
}

/**
 * @brief Ensures the chunk has enough capacity to store line information for a new line.
 * 
 * This function checks if the current line is different from the last recorded line,
 * or if it's the first line being added. If so, it grows the lines array if necessary
 * and adds a new entry for the line.
 *
 * @param chunk Pointer to the Chunk to ensure line capacity for.
 * @param line The line number to be added.
 */
void ensureLinesCapacity(Chunk* chunk, int line)
{
    if (chunk->maxLines == 0 || line != chunk->lines[chunk->currentLine]) {
        if (chunk->maxLines == 0) {
            chunk->maxLines = GROW_CAPACITY(0);
            chunk->lines = GROW_ARRAY(int, chunk->lines, 0, chunk->maxLines);
        } else {
            chunk->currentLine += 2;
            if (chunk->currentLine > chunk->maxLines) {
                int oldCapacity = chunk->maxLines;
                chunk->maxLines = GROW_CAPACITY(oldCapacity);
                chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->maxLines);
            }
        }
        chunk->lines[chunk->currentLine] = line;
        chunk->lines[chunk->currentLine + 1] = 0;
    }
}

/**
 * @brief Increments the count of instructions for the current line.
 * 
 * This function increases the count of instructions associated with the current line
 * in the chunk's line information array.
 *
 * @param chunk Pointer to the Chunk to increment the line count for.
 */
void incrementLineCount(Chunk* chunk)
{
    chunk->lines[chunk->currentLine + 1]++;
}

void
writeChunk(Chunk* chunk, uint8_t byte, int line)
{
    ensureCapacity(chunk, chunk->count + 1);
    ensureLinesCapacity(chunk, line);

    chunk->code[chunk->count] = byte;
    chunk->count++;
    incrementLineCount(chunk);
}

int
addConstant(Chunk* chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void writeConstant(Chunk* chunk, Value value, int line) {
    int constant = addConstant(chunk, value);
    OpCode op = OP_CONSTANT;
    if (value >= 256) op = OP_CONSTANT_LONG;
    writeChunk(chunk, op, line);
    writeChunk(chunk, constant, line);
}

void
freeChunk(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(uint8_t, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}