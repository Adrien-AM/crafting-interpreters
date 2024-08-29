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

ObjString* createString(int length) { return allocateString(length); }

ObjString* copyString(const char* chars, int length) {
    ObjString* string = allocateString(length);
    memcpy(string->chars, chars, length + 1);
    string->chars[length] = '\0';

    return string;
}

void printObject(Value value) {
    if (!IS_OBJ(value)) {
    }
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
    }
}