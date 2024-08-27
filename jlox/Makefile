JAVAC = javac
JAVA = java
SRC_DIR = .
TOOLS_DIR = ../tools
BIN_DIR = bin
TESTS_DIR = ../tests
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

gen: tools
	@$(JAVA) -cp $(BIN_DIR) $(TOOLS_DIR).GenerateAst $(SRC_DIR)

test: build
	@failed=0; \
	for file in $(TESTS_DIR)/*.lox; do \
		echo -n "Testing file $$file  "; \
		if $(MAKE) -s run ARGS="$$file"; then \
			echo -e "\x1b[32mPassed !\x1b[37m"; \
		else \
			echo -e "\x1b[31mFailed !\x1b[37m"; \
			failed=1; \
		fi; \
	done; \
	exit $$failed
	
clean:
	@rm -rf $(BIN_DIR)

.PHONY: all build tools run clean
