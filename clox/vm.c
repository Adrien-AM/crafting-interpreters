#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "vm.h"

VM vm;

static inline Value clockNative(int argCount, Value* args) {
    (void)argCount;
    (void)args;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static inline void resetStack() { vm.stackTop = vm.stack; }

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        // -1 because the IP is sitting on the next instruction to be
        // executed.
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
                getLine(&function->chunk, instruction));
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static inline Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            case OBJ_CLOSURE: return call(AS_CLOSURE(callee), argCount);
            default: break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }
    ObjUpvalue* createdUpvalue = newUpvalue(local);

    createdUpvalue->next = upvalue;
    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}

static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void toString(int distance) {
    Value value = peek(distance);
    if (!IS_STRING(value)) {
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
            runtimeError("Cannot convert object value to string.");
            return;
        }

        // side note : no need to worry about freeing anything, it can't be an
        // obj :)
        vm.stackTop[-1 - distance] = OBJ_VAL(string);
    }
}

static void concatenate() {
    // Keep them on the stack to avoid getting GC'ed
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char str[length];

    memcpy(str, a->chars, a->length);
    memcpy(str + a->length, b->chars, b->length);
    str[length] = '\0';

    ObjString* result = takeString(str, length);

    // needed ? those are not variable strings but raw data
    tableSet(&vm.strings, result, NIL_VAL);

    pop();
    pop();
    push(OBJ_VAL(result));
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
    vm.frameCount = 0;
    defineNative("clock", clockNative);
    vm.openUpvalues = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.currentGC = 0;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
}

InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    register uint8_t* ip = frame->ip;
#define READ_BYTE() (*ip++)
#define READ_CONSTANT()                                                        \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define BINARY_OP(valueType, op)                                               \
    do {                                                                       \
        if (__builtin_expect(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)),       \
                             false)) {                                         \
            runtimeError("Operands must be numbers.");                         \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(valueType(a op b));                                               \
    } while (false)

    // Caching
    Entry lastGlobalAccessed = {NULL, {0}};

#ifdef DEBUG_PROFILE_CODE
    int counts[OP_CALL + 1] = {0};
#endif
    for (;;) {
        __builtin_prefetch(&vm.stackTop[-1], 0, 3);
        __builtin_prefetch(frame->closure->function->chunk.constants.values, 0,
                           3);
#ifdef DEBUG_TRACE_EXECUTION
        printf(" ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(
            &frame->closure->function->chunk,
            (int)(ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction = READ_BYTE();
#ifdef DEBUG_PROFILE_CODE
        counts[instruction]++;
#endif
        switch (instruction) {
            case OP_CONSTANT_LONG:
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_NEGATE:
                *(vm.stackTop - 1) = NUMBER_VAL(-AS_NUMBER(*(vm.stackTop - 1)));
                break;
            case OP_ADD: {
                if (__builtin_expect(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)),
                                     true)) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    if (!IS_STRING(peek(0))) {
                        toString(0);
                    }
                    if (!IS_STRING(peek(1))) {
                        toString(1);
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
                if (__builtin_expect(tableSet(&vm.globals, name, peek(0)),
                                     false)) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable %s\n", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                frame->ip = ip;
                if (__builtin_expect(!callValue(peek(argCount), argCount),
                                     false)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));

                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();

                    if (isLocal) {
                        closure->upvalues[i] =
                            captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
            case OP_RETURN: {
                Value result = pop();

                closeUpvalues(frame->slots);

                vm.frameCount--;
                if (__builtin_expect(vm.frameCount == 0, false)) {
                    pop();
#ifdef DEBUG_PROFILE_CODE
                    printf("Instruction counts:\n");
                    for (int i = 0; i <= OP_CALL; i++) {
                        if (counts[i] > 0) {
                            printf("  %-15s: %d\n", opCodeToString(i),
                                   counts[i]);
                        }
                    }
#endif
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);

                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
#undef BINARY_OP
}

InterpretResult interpret(const char* source, bool saveCode) {
    ObjFunction* function = compile(source);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    callValue(OBJ_VAL(closure), 0);

    if (saveCode)
        writeFunctionToFile(function, "out.x7");

    return run();
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