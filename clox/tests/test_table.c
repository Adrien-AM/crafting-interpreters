#include <stdio.h>
#include <string.h>
#include "table.h"
#include "object.h"
#include "memory.h"
#include "test_utils.c"

static ObjString* createTestString(const char* chars) {
    return copyString(chars, strlen(chars));
}

TEST(InitTable) {
    Table table;
    initTable(&table);
    ASSERT_EQUAL(0, table.count);
    ASSERT_EQUAL(0, table.capacity);
    ASSERT(table.entries == NULL);
}

TEST(TableSetAndGet) {
    Table table;
    initTable(&table);

    ObjString* key = createTestString("test_key");
    Value value = NUMBER_VAL(42.0);

    bool isNewKey = tableSet(&table, key, value);
    ASSERT(isNewKey);

    Value retrievedValue;
    bool found = tableGet(&table, key, &retrievedValue);
    ASSERT(found);
    ASSERT_FLOAT_EQUAL(42.0, AS_NUMBER(retrievedValue), 0.0001);

    freeTable(&table);
}

TEST(TableDelete) {
    Table table;
    initTable(&table);

    ObjString* key = createTestString("delete_me");
    tableSet(&table, key, NUMBER_VAL(1.0));

    bool deleted = tableDelete(&table, key);
    ASSERT(deleted);

    Value value;
    bool found = tableGet(&table, key, &value);
    ASSERT(!found);

    freeTable(&table);
}

TEST(TableAddAll) {
    Table source, destination;
    initTable(&source);
    initTable(&destination);

    ObjString* key1 = createTestString("key1");
    ObjString* key2 = createTestString("key2");
    tableSet(&source, key1, NUMBER_VAL(1.0));
    tableSet(&source, key2, NUMBER_VAL(2.0));

    tableAddAll(&source, &destination);

    Value value;
    ASSERT(tableGet(&destination, key1, &value));
    ASSERT_FLOAT_EQUAL(1.0, AS_NUMBER(value), 0.0001);
    ASSERT(tableGet(&destination, key2, &value));
    ASSERT_FLOAT_EQUAL(2.0, AS_NUMBER(value), 0.0001);

    freeTable(&source);
    freeTable(&destination);
}

TEST(TableFindString) {
    Table table;
    initTable(&table);

    const char* testString = "findme";
    ObjString* key = copyString(testString, strlen(testString));
    tableSet(&table, key, NUMBER_VAL(1.0));

    ObjString* found = tableFindString(&table, testString, strlen(testString), key->hash);
    ASSERT_EQUAL(key, found);

    found = tableFindString(&table, "notfound", 8, 0);
    ASSERT(found == NULL);

    freeTable(&table);
}

int main() {
    printf("Running table tests...\n");

    RUN_TEST(InitTable);
    RUN_TEST(TableSetAndGet);
    RUN_TEST(TableDelete);
    RUN_TEST(TableAddAll);
    RUN_TEST(TableFindString);

    printf("All table tests completed.\n");
    return 0;
}
