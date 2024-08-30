#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../object.h"
#include "../vm.h"
#include "test_utils.c"

TEST(copyString) {
    VM vm;
    initVM(&vm);

    const char* testStr = "Hello, World!";
    int length = strlen(testStr);

    ObjString* str = copyString(testStr, length);

    ASSERT(str != NULL);
    ASSERT_EQUAL(str->length, length);
    ASSERT_STRING_EQUAL(str->chars, testStr);
    ASSERT_EQUAL(str->obj.type, OBJ_STRING);

    freeVM(&vm);
}

TEST(takeString) {
    VM vm;
    initVM(&vm);

    char* testStr = malloc(14);
    strcpy(testStr, "Hello, World!");
    int length = strlen(testStr);

    ObjString* str = takeString(testStr, length);

    ASSERT(str != NULL);
    ASSERT_EQUAL(str->length, length);
    ASSERT_STRING_EQUAL(str->chars, "Hello, World!");
    ASSERT_EQUAL(str->obj.type, OBJ_STRING);

    freeVM(&vm);
}

TEST(printObject) {
    VM vm;
    initVM(&vm);

    const char* testStr = "Test String";
    int length = strlen(testStr);

    ObjString* str = copyString(testStr, length);
    Value strValue = OBJ_VAL(str);

    printf("Printing object: ");
    printObject(strValue);
    printf("\n");

    freeVM(&vm);
}

int main() {
    RUN_TEST(copyString);
    RUN_TEST(takeString);
    RUN_TEST(printObject);

    printf("All tests passed!\n");
    return 0;
}
