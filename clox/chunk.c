#include <stdlib.h>
#include <stdio.h>

#include "chunk.h"
#include "object.h"

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
    if (chunk->constants.count >= 256) op = OP_CONSTANT_LONG;
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

void writeChunkToFile(Chunk* chunk, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return;
    }

    // Write chunk count and capacity
    fwrite(&chunk->count, sizeof(int), 1, file);
    fwrite(&chunk->capacity, sizeof(int), 1, file);

    // Write bytecode
    fwrite(chunk->code, sizeof(uint8_t), chunk->count, file);

    // Write line information
    fwrite(&chunk->maxLines, sizeof(int), 1, file);
    fwrite(chunk->lines, sizeof(int), chunk->maxLines, file);

    // Write constants
    fwrite(&chunk->constants.count, sizeof(int), 1, file);
    for (int i = 0; i < chunk->constants.count; i++) {
        Value value = chunk->constants.values[i];
        fwrite(&value.type, sizeof(ValueType), 1, file);
        switch (value.type) {
            case VAL_BOOL:
                fwrite(&value.as.boolean, sizeof(bool), 1, file);
                break;
            case VAL_NIL:
                // No data to write for nil
                break;
            case VAL_NUMBER:
                fwrite(&value.as.number, sizeof(double), 1, file);
                break;
            case VAL_OBJ:
                // For objects, we need to serialize based on the object type
                Obj* obj = AS_OBJ(value);
                fwrite(&obj->type, sizeof(ObjType), 1, file);
                if (obj->type == OBJ_STRING) {
                    ObjString* string = (ObjString*)obj;
                    fwrite(&string->length, sizeof(int), 1, file);
                    fwrite(string->chars, sizeof(char), string->length, file);
                }
                // Add cases for other object types as needed
                break;
        }
    }

    fclose(file);
}

Chunk* readChunkFromFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
        return NULL;
    }

    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    initChunk(chunk);

    // Read chunk count and capacity
    fread(&chunk->count, sizeof(int), 1, file);
    fread(&chunk->capacity, sizeof(int), 1, file);

    // Read bytecode
    chunk->code = (uint8_t*)reallocate(chunk->code, 0, chunk->capacity * sizeof(uint8_t));
    fread(chunk->code, sizeof(uint8_t), chunk->count, file);

    // Read line information
    fread(&chunk->maxLines, sizeof(int), 1, file);
    chunk->lines = (int*)reallocate(chunk->lines, 0, chunk->maxLines * sizeof(int));
    fread(chunk->lines, sizeof(int), chunk->maxLines, file);

    // Read constants
    int constantCount;
    fread(&constantCount, sizeof(int), 1, file);
    for (int i = 0; i < constantCount; i++) {
        ValueType type;
        fread(&type, sizeof(ValueType), 1, file);
        Value value;
        value.type = type;
        switch (type) {
            case VAL_BOOL:
                fread(&value.as.boolean, sizeof(bool), 1, file);
                break;
            case VAL_NIL:
                // No data to read for nil
                break;
            case VAL_NUMBER:
                fread(&value.as.number, sizeof(double), 1, file);
                break;
            case VAL_OBJ:
                ObjType objType;
                fread(&objType, sizeof(ObjType), 1, file);
                if (objType == OBJ_STRING) {
                    int length;
                    fread(&length, sizeof(int), 1, file);
                    char* chars = (char*)malloc(length + 1);
                    fread(chars, sizeof(char), length, file);
                    chars[length] = '\0';
                    value = OBJ_VAL(copyString(chars, length));
                    free(chars);
                }
                // Add cases for other object types as needed
                break;
        }
        writeValueArray(&chunk->constants, value);
    }

    fclose(file);
    return chunk;
}
