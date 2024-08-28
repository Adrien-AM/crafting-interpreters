#include "vm.h"
#include "debug.h"

#include <stdio.h>

VM vm;

static void
resetStack()
{
    vm.stackTop = vm.stack;
}

void
initVM()
{
    resetStack();
}

InterpretResult
run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// Using a do while loop in the macro looks funny, but it gives you a way to contain multiple statements inside a block that also permits a semicolon at the end.
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
    } while (false)

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
                case OP_NEGATE: *(vm.stackTop - 1) = -*(vm.stackTop - 1); break;
                case OP_ADD: BINARY_OP(+); break;
                case OP_SUBTRACT: BINARY_OP(-); break;
                case OP_MULTIPLY: BINARY_OP(*); break;
                case OP_DIVIDE: BINARY_OP(/); break;
                case OP_RETURN:
                    printValue(pop());
                    printf("\n");
                    return INTERPRET_OK;
            }
        }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult
interpret(Chunk* chunk)
{
    vm.chunk = chunk;
    vm.ip = chunk->code;
    return run();
}

void
push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value
pop()
{
    // if (vm.stackTop == vm.stack)
    vm.stackTop--;
    return *vm.stackTop;
}

void
freeVM()
{
}