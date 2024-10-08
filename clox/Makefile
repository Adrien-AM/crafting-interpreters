CC = gcc
CFLAGS = -Wall -Wextra -std=c23 -O3
SRCS = $(wildcard *.c)
BUILD_DIR = build
OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
TARGET = $(BUILD_DIR)/clox

.PHONY: all clean run mem test prof

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: $(TARGET)
	@./$(TARGET) $(ARGS)

mem: $(TARGET)
	@valgrind --leak-check=full --track-origins=yes ./$(TARGET) $(ARGS)

prof: CFLAGS += -pg
prof: clean $(TARGET)
	@./$(TARGET) $(ARGS)
	@gprof $(TARGET) gmon.out > profile_output.txt
	@less profile_output.txt


TEST_SRCS = $(wildcard tests/*.c)
TEST_TARGETS = $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRCS))

TEST_SRCS := $(filter-out tests/test_utils.c, $(TEST_SRCS))
TEST_TARGETS := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRCS))

test: $(TEST_TARGETS)
	@for test in $(TEST_TARGETS); do \
		echo "== Running $$test =="; \
		$$test; \
		echo ""; \
	done

$(BUILD_DIR)/tests/%: tests/%.c $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) | $(BUILD_DIR)/tests
	@$(CC) $(CFLAGS) -I. -o $@ $^

$(BUILD_DIR)/tests:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) vgcore.* gmon.out

