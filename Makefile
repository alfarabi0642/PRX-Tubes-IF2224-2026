CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17

BIN_DIR := bin
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BIN_DIR)/arion.exe

SRCS := \
	src/main.cpp \
	src/lexer/lexer.cpp \
	src/common/utils.cpp \
	src/common/token.cpp \
	src/parser/parser_core.cpp \
	src/parser/parser_toplevel.cpp \
	src/parser/parser_declarations.cpp \
	src/parser/parser_statements.cpp \
	src/parser/parser_expressions.cpp

OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(INPUT)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
