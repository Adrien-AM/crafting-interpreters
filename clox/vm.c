#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "vm.h"

VM vm;

static inline void resetStack() { vm.stackTop = vm.stack; }

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = getLine(vm.chunk, instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

static inline Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void toString() {
    if (!IS_STRING(peek(0))) {
        Value value = pop();
        ObjString* string;

        if (IS_NUMBER(value)) {
            char buffer[24];
            int length =
                snprintf(buffer, sizeof(buffer), "%.1f", AS_NUMBER(value));
            string = copyString(buffer, length);
        } else if (IS_BOOL(value)) {
            string =
                AS_BOOL(value) ? copyString("true", 4) : copyString("false", 5);
        } else if (IS_NIL(value)) {
            string = copyString("nil", 3);
        } else {
            runtimeError("Cannot convert value to string.");
            return;
        }

        push(OBJ_VAL(string));
    }
}

static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char str[length];

    memcpy(str, a->chars, a->length);
    memcpy(str + a->length, b->chars, b->length);
    str[length] = '\0';

    ObjString* result = takeString(str, length);

    // needed ? those are not variable strings but raw data
    tableSet(&vm.strings, result, NIL_VAL);

    push(OBJ_VAL(result));
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
}

InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
// Using a do while loop in the macro looks funny, but it gives you a way to
// contain multiple statements inside a block that also permits a semicolon at
// the end.
#define BINARY_OP(valueType, op)                                               \
    do {                                                                       \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                      \
            runtimeError("Operands must be numbers.");                         \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(valueType(a op b));                                               \
    } while (false)

    Entry lastGlobalAccessed = {NULL, {0}}; // Caching

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf(" ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT_LONG:
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_NEGATE:
                *(vm.stackTop - 1) = NUMBER_VAL(-AS_NUMBER(*(vm.stackTop - 1)));
                break;
            case OP_ADD: {
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    // TODO: optimize that to avoid one push/pop
                    if (!IS_STRING(peek(0))) {
                        toString();
                    }
                    if (!IS_STRING(peek(1))) {
                        Value tmp = pop();
                        toString();
                        push(tmp);
                    }
                    concatenate();
                } else {
                    runtimeError("Operands must be two numbers or one of them "
                                 "must be a strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_ASSERT:
                if (isFalsey(pop()))
                    runtimeError("Assertion error.");
                break;
            case OP_POP: pop(); break;
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;

                if (lastGlobalAccessed.key != NULL &&
                    stringsEqual(lastGlobalAccessed.key, name)) {
                    value = lastGlobalAccessed.value;
                } else if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable %s.\n", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                lastGlobalAccessed.key = name;
                lastGlobalAccessed.value = value;
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (lastGlobalAccessed.key != NULL &&
                    stringsEqual(lastGlobalAccessed.key, name)) {
                    lastGlobalAccessed.value = peek(0);
                }
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable %s\n", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_JUMP: {
                uint8_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint8_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    vm.ip += offset;
                break;
            }
            case OP_LOOP: {
                uint8_t offset = READ_SHORT();
                vm.ip -= offset;
                break;
            }
            case OP_RETURN: return INTERPRET_OK;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
#undef BINARY_OP
}

InterpretResult interpret(const char* source, bool saveChunk) {
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    if (saveChunk)
        writeChunkToFile(&chunk, "out.chunk");

    InterpretResult result = run();
    freeChunk(&chunk);
    return result;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    // if (vm.stackTop == vm.stack)
    vm.stackTop--;
    return *vm.stackTop;
}

void freeVM() {
    freeObjects();
    freeTable(&vm.strings);
    freeTable(&vm.globals);
}