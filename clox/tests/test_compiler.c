#include <stdio.h>
#include <string.h>
#include "../compiler.h"
#include "../chunk.h"
#include "test_utils.c"

TEST(compile_simple_expression) {
    const char* source = "2 + 3;";
    Chunk chunk;
    initChunk(&chunk);

    bool result = compile(source, &chunk);

    ASSERT(result);
    ASSERT_EQUAL(7, chunk.count);  // 2*2 for constants, 1 for OP_ADD, 1 for OP_RETURN, 1 for OP_POP

    ASSERT_EQUAL(OP_CONSTANT, chunk.code[0]);
    ASSERT_EQUAL(OP_CONSTANT, chunk.code[2]);
    ASSERT_EQUAL(OP_ADD, chunk.code[4]);
    ASSERT_EQUAL(OP_POP, chunk.code[5]);
    ASSERT_EQUAL(OP_RETURN, chunk.code[6]);

    ASSERT_FLOAT_EQUAL(2.0, AS_NUMBER(chunk.constants.values[0]), 0.0001);
    ASSERT_FLOAT_EQUAL(3.0, AS_NUMBER(chunk.constants.values[1]), 0.0001);

    freeChunk(&chunk);
}

TEST(compile_complex_expression) {
    const char* source = "2 * (3 + 4) - 5;";
    Chunk chunk;
    initChunk(&chunk);

    bool result = compile(source, &chunk);

    ASSERT(result);
    ASSERT_EQUAL(13, chunk.count);  // 4*2 constants, 3 operations, 1 return, 1 pop

    ASSERT_EQUAL(OP_CONSTANT, chunk.code[0]);
    ASSERT_EQUAL(OP_CONSTANT, chunk.code[2]);
    ASSERT_EQUAL(OP_CONSTANT, chunk.code[4]);
    ASSERT_EQUAL(OP_ADD, chunk.code[6]);
    ASSERT_EQUAL(OP_MULTIPLY, chunk.code[7]);
    ASSERT_EQUAL(OP_CONSTANT, chunk.code[8]);
    ASSERT_EQUAL(OP_SUBTRACT, chunk.code[10]);
    ASSERT_EQUAL(OP_POP, chunk.code[11]);
    ASSERT_EQUAL(OP_RETURN, chunk.code[12]);

    ASSERT_FLOAT_EQUAL(2.0, AS_NUMBER(chunk.constants.values[0]), 0.0001);
    ASSERT_FLOAT_EQUAL(3.0, AS_NUMBER(chunk.constants.values[1]), 0.0001);
    ASSERT_FLOAT_EQUAL(4.0, AS_NUMBER(chunk.constants.values[2]), 0.0001);
    ASSERT_FLOAT_EQUAL(5.0, AS_NUMBER(chunk.constants.values[3]), 0.0001);

    freeChunk(&chunk);
}

TEST(compile_invalid_expression) {
    const char* source = "2 + * 3;";
    Chunk chunk;
    initChunk(&chunk);

    bool result = compile(source, &chunk);

    ASSERT(!result);

    freeChunk(&chunk);
}

int main() {
    printf("Running compiler tests...\n");

    RUN_TEST(compile_simple_expression);
    RUN_TEST(compile_complex_expression);
    RUN_TEST(compile_invalid_expression);

    return 0;
}
