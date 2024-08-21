JAVAC = javac
JAVA = java
SRC_DIR = jlox
TOOLS_DIR = tools
BIN_DIR = bin
MAIN_CLASS = jlox.Lox

all: build tools

build:
	@mkdir -p $(BIN_DIR)
	@$(JAVAC) -d $(BIN_DIR) $(SRC_DIR)/*.java

tools:
	@mkdir -p $(BIN_DIR)
	@$(JAVAC) -d $(BIN_DIR) $(TOOLS_DIR)/*.java

run: build
	@$(JAVA) -cp $(BIN_DIR) $(MAIN_CLASS) $(ARGS)

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all build tools run clean
