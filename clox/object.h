#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
    Obj* next; // Instrusive list pattern for garbage collection
};

struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    char chars[]; // Flexible array member
};

ObjString* takeString(const char* chars, int length);
ObjString* copyString(const char* chars, int length);
bool stringsEqual(ObjString* a, ObjString* b);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) (isObjType(value, OBJ_STRING))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#endif