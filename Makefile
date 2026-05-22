CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17
POWERSHELL := powershell -NoProfile -Command

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
	src/parser/parser_expressions.cpp \
	src/semantic/semantic.cpp

OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(POWERSHELL) "New-Item -ItemType Directory -Force -Path '$(BIN_DIR)' | Out-Null"
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: src/%.cpp
	@$(POWERSHELL) "New-Item -ItemType Directory -Force -Path '$(dir $@)' | Out-Null"
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(INPUT)

clean:
	@$(POWERSHELL) "if (Test-Path '$(BUILD_DIR)') { Remove-Item -Recurse -Force '$(BUILD_DIR)' }; if (Test-Path '$(TARGET)') { Remove-Item -Force '$(TARGET)' }"
