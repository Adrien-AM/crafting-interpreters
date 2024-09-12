#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/**
 * @struct Entry
 * @brief Represents a key-value pair in the hash table.
 *
 * @field key Pointer to the string object used as the key.
 * @field value The value associated with the key.
 */
typedef struct {
    ObjString* key;
    Value value;
} Entry;

/**
 * @struct Table
 * @brief Represents a hash table data structure.
 *
 * @field count The number of entries in the table.
 * @field capacity The current capacity of the table.
 * @field entries Pointer to the array of entries.
 */
typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

/**
 * @brief Initializes a new hash table.
 * @param table Pointer to the Table structure to initialize.
 */
void initTable(Table* table);

/**
 * @brief Frees the memory allocated for the hash table.
 * @param table Pointer to the Table structure to free.
 */
void freeTable(Table* table);

/**
 * @brief Retrieves a value from the hash table.
 * @param table Pointer to the Table structure.
 * @param key The key to look up.
 * @param value Pointer to store the retrieved value.
 * @return true if the key was found, false otherwise.
 */
bool tableGet(Table* table, ObjString* key, Value* value);

/**
 * @brief Sets a key-value pair in the hash table.
 * @param table Pointer to the Table structure.
 * @param key The key to set.
 * @param value The value to associate with the key.
 * @return true if a new entry was added, false if an existing entry was updated.
 */
bool tableSet(Table* table, ObjString* key, Value value);

/**
 * @brief Deletes a key-value pair from the hash table.
 * @param table Pointer to the Table structure.
 * @param key The key to delete.
 * @return true if the key was found and deleted, false otherwise.
 */
bool tableDelete(Table* table, ObjString* key);

/**
 * @brief Adds all entries from one table to another.
 * @param from Pointer to the source Table.
 * @param to Pointer to the destination Table.
 */
void tableAddAll(Table* from, Table* to);

/**
 * @brief Finds a string in the table based on its contents and hash.
 * @param table Pointer to the Table structure.
 * @param chars The character array to search for.
 * @param length The length of the character array.
 * @param hash The hash of the string.
 * @return Pointer to the found ObjString, or NULL if not found.
 */
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

/**
 * @brief Marks all objects in the table for garbage collection.
 * @param table Pointer to the Table structure.
 */
void markTable(Table* table);

/**
 * @brief Removes all white (unmarked) objects from the table.
 * @param table Pointer to the Table structure.
 */
void tableRemoveWhites(Table* table);

#endif