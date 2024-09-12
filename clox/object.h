#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

/**
 * @enum ObjType
 * @brief Enumerates the different types of objects in the VM.
 */
typedef enum : char {
    OBJ_STRING,         /**< String object */
    OBJ_FUNCTION,       /**< Function object */
    OBJ_NATIVE,         /**< Native function object */
    OBJ_CLOSURE,        /**< Closure object */
    OBJ_UPVALUE,        /**< Upvalue object */
    OBJ_CLASS,          /**< Class object */
    OBJ_INSTANCE,       /**< Instance object */
    OBJ_BOUND_METHOD,   /**< Bound method object */
} ObjType;

/**
 * @struct Obj
 * @brief Base structure for all object types.
 */
struct Obj {
    ObjType type;       /**< Type of the object */
    short lastCollect;  /**< Last garbage collection cycle this object was marked in */
    Obj* next;          /**< Next object in the intrusive list for garbage collection */
};

/**
 * @struct ObjFunction
 * @brief Represents a function object.
 */
typedef struct {
    Obj obj;            /**< Base object */
    int arity;          /**< Number of parameters the function expects */
    Chunk chunk;        /**< Chunk of bytecode for the function */
    ObjString* name;    /**< Name of the function */
    int upvalueCount;   /**< Number of upvalues the function closes over */
} ObjFunction;

/**
 * @typedef NativeFn
 * @brief Function pointer type for native functions.
 */
typedef Value (*NativeFn)(int argCount, Value* args);

/**
 * @struct ObjNative
 * @brief Represents a native function object.
 */
typedef struct {
    Obj obj;            /**< Base object */
    NativeFn function;  /**< Pointer to the native function */
} ObjNative;

/**
 * @struct ObjUpvalue
 * @brief Represents an upvalue object.
 */
struct ObjUpvalue {
    Obj obj;            /**< Base object */
    Value* location;    /**< Pointer to the variable this upvalue closes over */
    Value closed;       /**< Closed-over value (used when the upvalue is closed) */
    struct ObjUpvalue* next; /**< Next upvalue in the list */
};

typedef struct ObjUpvalue ObjUpvalue;

/**
 * @struct ObjClosure
 * @brief Represents a closure object.
 */
typedef struct {
    Obj obj;            /**< Base object */
    ObjFunction* function; /**< The function this closure wraps */
    ObjUpvalue** upvalues; /**< Array of upvalues */
    int upvalueCount;   /**< Number of upvalues */
} ObjClosure;

/**
 * @struct ObjString
 * @brief Represents a string object.
 */
struct ObjString {
    Obj obj;            /**< Base object */
    int length;         /**< Length of the string */
    uint32_t hash;      /**< Hash of the string */
    char chars[];       /**< Flexible array member holding the string contents */
};

/**
 * @struct ObjClass
 * @brief Represents a class object.
 */
typedef struct {
    Obj obj;            /**< Base object */
    ObjString* name;    /**< Name of the class */
    Table methods;      /**< Table of methods */
} ObjClass;

/**
 * @struct ObjInstance
 * @brief Represents an instance of a class.
 */
typedef struct {
    Obj obj;            /**< Base object */
    ObjClass* klass;    /**< Class of this instance */
    Table fields;       /**< Table of fields */
} ObjInstance;

/**
 * @struct ObjBoundMethod
 * @brief Represents a bound method object.
 */
typedef struct {
    Obj obj;            /**< Base object */
    Value receiver;     /**< The receiver (instance) the method is bound to */
    ObjClosure* method; /**< The method closure */
} ObjBoundMethod;

/**
 * @brief Creates a new function object.
 * @return Pointer to the new ObjFunction.
 */
ObjFunction* newFunction();

/**
 * @brief Creates a new native function object.
 * @param function Pointer to the native function.
 * @return Pointer to the new ObjNative.
 */
ObjNative* newNative(NativeFn function);

/**
 * @brief Creates a new closure object.
 * @param function The function to create a closure for.
 * @return Pointer to the new ObjClosure.
 */
ObjClosure* newClosure(ObjFunction* function);

/**
 * @brief Creates a new upvalue object.
 * @param slot Pointer to the Value this upvalue closes over.
 * @return Pointer to the new ObjUpvalue.
 */
ObjUpvalue* newUpvalue(Value* slot);

/**
 * @brief Creates a new class object.
 * @param name The name of the class.
 * @return Pointer to the new ObjClass.
 */
ObjClass* newClass(ObjString* name);

/**
 * @brief Creates a new instance object.
 * @param klass The class to instantiate.
 * @return Pointer to the new ObjInstance.
 */
ObjInstance* newInstance(ObjClass* klass);

/**
 * @brief Creates a new bound method object.
 * @param receiver The receiver (instance) to bind the method to.
 * @param method The method closure.
 * @return Pointer to the new ObjBoundMethod.
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);

/**
 * @brief Creates a new string object, taking ownership of the given char array.
 * @param chars The character array to use (will be freed by the VM).
 * @param length The length of the string.
 * @return Pointer to the new ObjString.
 */
ObjString* takeString(const char* chars, int length);

/**
 * @brief Creates a new string object, copying the given char array.
 * @param chars The character array to copy.
 * @param length The length of the string.
 * @return Pointer to the new ObjString.
 */
ObjString* copyString(const char* chars, int length);

/**
 * @brief Compares two string objects for equality.
 * @param a First string to compare.
 * @param b Second string to compare.
 * @return true if the strings are equal, false otherwise.
 */
bool stringsEqual(ObjString* a, ObjString* b);

/**
 * @brief Prints a representation of the object to stdout.
 * @param value The Value containing the object to print.
 */
void printObject(Value value);

/**
 * @brief Writes a function object to a file.
 * @param function The function to write.
 * @param filename The name of the file to write to.
 */
void writeFunctionToFile(ObjFunction* function, const char* filename);

/**
 * @brief Reads a function object from a file.
 * @param filename The name of the file to read from.
 * @return Pointer to the read ObjFunction, or NULL if reading failed.
 */
ObjFunction* readFunctionFromFile(const char* filename);

/**
 * @brief Checks if a Value is of a specific ObjType.
 * @param value The Value to check.
 * @param type The ObjType to check against.
 * @return true if the Value is an object of the specified type, false otherwise.
 */
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value) (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value) (isObjType(value, OBJ_NATIVE))
#define IS_CLOSURE(value) (isObjType(value, OBJ_CLOSURE))
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))

#endif