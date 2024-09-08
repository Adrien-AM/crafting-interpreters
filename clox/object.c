
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    object->lastCollect = 0;
    vm.objects = object;
// #ifdef DEBUG_LOG_GC
//     printf("%p allocate %ld for %d\n", (void*)object, size, type);
// #endif
    return object;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

    function->arity = 0;
    function->name = NULL;
    function->upvalueCount = 0;
    initChunk(&function->chunk);
    return function;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    memset(upvalues, 0, sizeof(ObjUpvalue*) * function->upvalueCount);

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

// Special case because of the use of Flexible Array Members
static ObjString* allocateString(int length) {
    ObjString* string = (ObjString*)allocateObject(
        sizeof(ObjString) + sizeof(char) * length + 1, OBJ_STRING);
    string->length = length;
    return string;
}

// FNV-1a algorithm
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString(const char* chars, int length) {
    // Avoid using this function as it makes an additional memcpy
    uint32_t hash = hashString(chars, length);

    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    ObjString* string = allocateString(length);
    memcpy(string->chars, chars, length * sizeof(char));
    string->chars[length] = '\0';
    string->hash = hash;

    return string;
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    ObjString* string = allocateString(length);
    memcpy(string->chars, chars, length + 1);
    string->chars[length] = '\0';
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();

    return string;
}

bool stringsEqual(ObjString* a, ObjString* b) {
    return (a->length == b->length && a->hash == b->hash &&
            memcmp(a->chars, b->chars, a->length) == 0);
}

void printObject(Value value) {
    if (!IS_OBJ(value)) {
    }
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
        case OBJ_NATIVE: printf("<native fn>"); break;
        case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function); break;
        case OBJ_UPVALUE: printf("upvalue"); break;
    }
}

void writeChunkToFile(Chunk* chunk, FILE* file);

void writeObjFunctionToFile(ObjFunction* function, FILE* file) {
    Obj fakeObj = {.type = OBJ_FUNCTION, .lastCollect = 0};
    Value fakeVal = {.type = VAL_OBJ, .as = {.obj = &fakeObj}};
    fwrite(&fakeVal, sizeof(Value), 1, file);

    // Write the arity
    fwrite(&function->arity, sizeof(int), 1, file);

    // Write the chunk
    writeChunkToFile(&function->chunk, file);
}

void writeChunkToFile(Chunk* chunk, FILE* file) {
    // Write the count and capacity
    fwrite(&chunk->count, sizeof(int), 1, file);
    fwrite(&chunk->capacity, sizeof(int), 1, file);

    // Write the code
    fwrite(chunk->code, sizeof(uint8_t), chunk->count, file);

    // Write the constants
    int constantsCount = chunk->constants.count;
    fwrite(&constantsCount, sizeof(int), 1, file);
    for (int i = 0; i < constantsCount; i++) {
        Value constant = chunk->constants.values[i];
        if (IS_FUNCTION(constant)) {
            ObjFunction* func = AS_FUNCTION(constant);
            writeObjFunctionToFile(func, file);
        } else {
            fwrite(&constant, sizeof(Value), 1, file);
        }
    }

    // Write the lines
    fwrite(chunk->lines, sizeof(int), chunk->currentLine, file);
}

void writeFunctionToFile(ObjFunction* function, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\" for writing.\n", filename);
        return;
    }

    writeObjFunctionToFile(function, file);

    fclose(file);
}

void readChunkFromFile(Chunk* chunk, FILE* file);

ObjFunction* readObjFunctionFromFile(FILE* file) {
    // Read the arity
    int arity;
    fread(&arity, sizeof(int), 1, file);

    // Read the chunk
    Chunk chunk;
    readChunkFromFile(&chunk, file);

    ObjFunction* function = newFunction();
    function->arity = arity;
    function->chunk = chunk;

    return function;
}

void readChunkFromFile(Chunk* chunk, FILE* file) {
    // Read the count and capacity
    fread(&chunk->count, sizeof(int), 1, file);
    fread(&chunk->capacity, sizeof(int), 1, file);

    // Allocate memory for the code
    chunk->code = (uint8_t*)malloc(chunk->capacity * sizeof(uint8_t));
    fread(chunk->code, sizeof(uint8_t), chunk->count, file);

    // Read the constants
    int constantsCount;
    fread(&constantsCount, sizeof(int), 1, file);
    initValueArray(&chunk->constants);
    for (int i = 0; i < constantsCount; i++) {
        Value constant;
        fread(&constant, sizeof(Value), 1, file);
        if (IS_FUNCTION(constant)) {
            ObjFunction* func = readObjFunctionFromFile(file);
            constant = OBJ_VAL(func);
        }
        writeValueArray(&chunk->constants, constant);
    }

    // Read the lines
    chunk->lines = (int*)malloc(chunk->count * sizeof(int));
    fread(chunk->lines, sizeof(int), chunk->count, file);
}

ObjFunction* readFunctionFromFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\" for reading.\n", filename);
        return NULL;
    }

    // Hack since main and other functions are slightly different
    Value v;
    fread(&v, sizeof(Value), 1, file);
    (void)v;

    ObjFunction* function = readObjFunctionFromFile(file);

    fclose(file);

    return function;
}
