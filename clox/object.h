#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

typedef enum { OBJ_STRING, OBJ_FUNCTION, OBJ_NATIVE, OBJ_CLOSURE, OBJ_UPVALUE } ObjType;

struct Obj {
    ObjType type;
    Obj* next; // Instrusive list pattern for garbage collection
};

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
    int upvalueCount;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
};

typedef struct ObjUpvalue ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    char chars[]; // Flexible array member
};


ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjClosure* newClosure(ObjFunction* function);
ObjUpvalue* newUpvalue(Value* slot);

ObjString* takeString(const char* chars, int length);
ObjString* copyString(const char* chars, int length);
bool stringsEqual(ObjString* a, ObjString* b);
void printObject(Value value);

void writeFunctionToFile(ObjFunction* function, const char* filename);
ObjFunction* readFunctionFromFile(const char* filename);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value) (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value) (isObjType(value, OBJ_NATIVE))
#define IS_CLOSURE(value) (isObjType(value, OBJ_CLOSURE))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

#endif