CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
SRCS = $(wildcard *.c)
BUILD_DIR = build
OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
TARGET = $(BUILD_DIR)/clox

.PHONY: all clean run mem

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

clean:
	rm -rf $(BUILD_DIR) vgcore.*

