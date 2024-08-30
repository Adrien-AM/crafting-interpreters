#include <stdio.h>
#include "../chunk.h"
#include "../value.h"
#include "test_utils.c"

TEST(initChunk) {
    Chunk chunk;
    initChunk(&chunk);
    
    ASSERT_EQUAL(0, chunk.count);
    ASSERT_EQUAL(0, chunk.capacity);
    ASSERT(chunk.code == NULL);
    ASSERT(chunk.lines == NULL);
    ASSERT_EQUAL(0, chunk.maxLines);
    ASSERT_EQUAL(0, chunk.currentLine);
    ASSERT_EQUAL(0, chunk.constants.count);
    
    freeChunk(&chunk);
}

TEST(writeChunk) {
    Chunk chunk;
    initChunk(&chunk);
    
    writeChunk(&chunk, OP_RETURN, 123);
    
    ASSERT_EQUAL(1, chunk.count);
    ASSERT_EQUAL(OP_RETURN, chunk.code[0]);
    ASSERT_EQUAL(123, getLine(&chunk, 0));
    
    freeChunk(&chunk);
}

TEST(addConstant) {
    Chunk chunk;
    initChunk(&chunk);
    
    int index = addConstant(&chunk, NUMBER_VAL(3.14));
    
    ASSERT_EQUAL(0, index);
    ASSERT_EQUAL(1, chunk.constants.count);
    ASSERT_FLOAT_EQUAL(3.14, AS_NUMBER(chunk.constants.values[0]), 0.0001);
    
    freeChunk(&chunk);
}

TEST(writeConstant) {
    Chunk chunk;
    initChunk(&chunk);
    
    writeConstant(&chunk, NUMBER_VAL(2.718), 456);
    
    ASSERT_EQUAL(2, chunk.count);  // OP_CONSTANT + constant index
    ASSERT_EQUAL(OP_CONSTANT, chunk.code[0]);
    ASSERT_EQUAL(0, chunk.code[1]);  // First constant index
    ASSERT_EQUAL(456, getLine(&chunk, 0));
    ASSERT_EQUAL(456, getLine(&chunk, 1));
    ASSERT_EQUAL(1, chunk.constants.count);
    ASSERT_FLOAT_EQUAL(2.718, AS_NUMBER(chunk.constants.values[0]), 0.0001);
    
    freeChunk(&chunk);
}

TEST(getLine) {
    Chunk chunk;
    initChunk(&chunk);
    
    writeChunk(&chunk, OP_RETURN, 100);
    writeChunk(&chunk, OP_CONSTANT, 100);
    writeChunk(&chunk, OP_ADD, 101);
    
    ASSERT_EQUAL(100, getLine(&chunk, 0));
    ASSERT_EQUAL(100, getLine(&chunk, 1));
    ASSERT_EQUAL(101, getLine(&chunk, 2));
    
    freeChunk(&chunk);
}

int main() {
    RUN_TEST(initChunk);
    RUN_TEST(writeChunk);
    RUN_TEST(addConstant);
    RUN_TEST(writeConstant);
    RUN_TEST(getLine);
    
    return 0;
}
