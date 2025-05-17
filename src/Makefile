CC = g++
CFLAGS = -Wall -std=c++17 -fpermissive -I.

FLEX = flex
BISON = bison

# Directory structure
SRC_DIR = .
BUILD_DIR = ../bin

# Output binary
OUTPUT = $(BUILD_DIR)/mikrotik_compiler

# Flex and Bison generated files
PARSER_C = $(BUILD_DIR)/parser.tab.c
PARSER_H = $(BUILD_DIR)/parser.tab.h
LEXER_C = $(BUILD_DIR)/lex.yy.c

# Source files (all cpp files in current directory)
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))

# Object files
OBJECTS = $(BUILD_DIR)/parser.tab.o $(BUILD_DIR)/lex.yy.o $(BUILD_DIR)/parser.o $(OBJ)

all: $(OUTPUT)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate parser files with Bison (generate the .h file first)
$(PARSER_H): parser.bison | $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_C) $<

# Generate parser files with Bison
$(PARSER_C): $(PARSER_H)
	@echo "Parser C already generated with header"

# Generate lexer files with Flex - depends on the header being generated first
$(LEXER_C): scanner.flex $(PARSER_H) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

# Compile C++ files from src directory
$(BUILD_DIR)/%.o: %.cpp $(PARSER_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile parser.c (main.c)
$(BUILD_DIR)/parser.o: main.c $(PARSER_H) $(LEXER_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile parser.tab.c
$(BUILD_DIR)/parser.tab.o: $(PARSER_C) $(PARSER_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile lex.yy.c
$(BUILD_DIR)/lex.yy.o: $(LEXER_C) $(PARSER_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(BUILD_DIR) -c $< -o $@

# Link all object files
$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean 