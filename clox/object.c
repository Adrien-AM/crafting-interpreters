#include <stdio.h>
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
    vm.objects = object;
    return object;
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

    tableSet(&vm.strings, string, NIL_VAL);

    return string;
}

void printObject(Value value) {
    if (!IS_OBJ(value)) {
    }
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
    }
}