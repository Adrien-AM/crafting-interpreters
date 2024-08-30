#include "../clox/object.h"
#include "../clox/table.h"
#include "../clox/value.h"
#include "../clox/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 10000000 // Increased size for more meaningful benchmarks

// Benchmark 1: Insertion performance
void
benchmarkInsertion()
{
    Table table;
    initTable(&table);

    clock_t start = clock();
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));
        tableSet(&table, keyString, NUMBER_VAL(i));
    }
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Insertion of %d items: %.2f seconds\n", SIZE, time_spent);

    freeTable(&table);
}

// Benchmark 2: Lookup performance
void
benchmarkLookup()
{
    Table table;
    initTable(&table);

    // Insert items
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));
        tableSet(&table, keyString, NUMBER_VAL(i));
    }

    clock_t start = clock();
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));
        Value value;
        tableGet(&table, keyString, &value);
    }
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Lookup of %d items: %.2f seconds\n", SIZE, time_spent);

    freeTable(&table);
}

// Benchmark 3: Deletion performance
void
benchmarkDeletion()
{
    Table table;
    initTable(&table);

    // Insert items
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));
        tableSet(&table, keyString, NUMBER_VAL(i));
    }

    clock_t start = clock();
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));
        tableDelete(&table, keyString);
    }
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Deletion of %d items: %.2f seconds\n", SIZE, time_spent);

    freeTable(&table);
}

// Benchmark 4: Mixed operations
void
benchmarkMixedOperations()
{
    Table table;
    initTable(&table);

    clock_t start = clock();
    for (int i = 0; i < SIZE; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        ObjString* keyString = copyString(key, strlen(key));

        if (i % 3 == 0) {
            // Insert
            tableSet(&table, keyString, NUMBER_VAL(i));
        } else if (i % 3 == 1) {
            // Lookup
            Value value;
            tableGet(&table, keyString, &value);
        } else {
            // Delete
            tableDelete(&table, keyString);
        }
    }
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Mixed operations (%d: 1/3 insert, 1/3 lookup, 1/3 delete): %.2f seconds\n",
           SIZE,
           time_spent);

    freeTable(&table);
}

int
main()
{
    printf("Running hash table benchmarks...\n");

    initVM();

    benchmarkInsertion();
    benchmarkLookup();
    benchmarkDeletion();
    benchmarkMixedOperations();

    freeVM();

    return 0;
}
