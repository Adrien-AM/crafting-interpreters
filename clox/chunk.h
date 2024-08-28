#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "memory.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_RETURN
} OpCode;

typedef struct
{
    // Dynamic array
    int count;
    int capacity;

    uint8_t* code;
    ValueArray constants;

    // Run-Length Encoding
    int* lines;
    int maxLines;
    int currentLine;
} Chunk;

/**
 * @brief Initializes a new Chunk.
 *
 * This function initializes a Chunk structure, setting all its members to initial values.
 * It sets count and capacity to 0, code and lines to NULL, maxLines and currentLine to 0,
 * and initializes the constants ValueArray.
 *
 * @param chunk Pointer to the Chunk to be initialized.
 */
void
initChunk(Chunk* chunk);

/**
 * @brief Retrieves the line number for a given instruction index.
 *
 * This function searches through the lines array to find the corresponding
 * line number for the given instruction index.
 *
 * @param chunk Pointer to the Chunk containing the instructions.
 * @param index The index of the instruction whose line number is to be retrieved.
 * @return The line number of the instruction at the given index, or -1 if not found.
 */
int
getLine(Chunk* chunk, int index);

/**
 * @brief Writes a byte of code to the chunk.
 *
 * This function adds a byte of code to the chunk, ensuring there's enough capacity
 * in the code array and updating the line information. It also increments the
 * count of instructions in the chunk.
 *
 * @param chunk Pointer to the Chunk to write to.
 * @param byte The byte of code to write.
 * @param line The line number of the source code this byte corresponds to.
 */
void
writeChunk(Chunk* chunk, uint8_t byte, int line);

/**
 * @brief Adds a constant to the chunk's constant pool.
 *
 * This function adds a constant value to the chunk's constant pool and
 * returns the index where it was added.
 *
 * @param chunk Pointer to the Chunk to add the constant to.
 * @param value The Value to add as a constant.
 * @return The index of the newly added constant in the constant pool.
 */
int
addConstant(Chunk* chunk, Value value);

void
writeConstant(Chunk* chunk, Value value, int line);

/**
 * @brief Frees the memory associated with a Chunk.
 *
 * This function deallocates all memory used by the Chunk, including its
 * code array, lines array, and constant pool. It then reinitializes the
 * Chunk to a clean state.
 *
 * @param chunk Pointer to the Chunk to be freed.
 */
void
freeChunk(Chunk* chunk);

#endif